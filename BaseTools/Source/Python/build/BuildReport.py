## @file
# Routines for generating build report.
#
# This module contains the functionality to generate build report after
# build all target completes successfully.
#
# Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#
import Common.LongFilePathOs as os
import re
import platform
import textwrap
import traceback
import sys
import time
import struct
import hashlib
import subprocess
import threading
from datetime import datetime
from io import BytesIO
from Common import EdkLogger
from Common.Misc import SaveFileOnChange
from Common.Misc import GuidStructureByteArrayToGuidString
from Common.Misc import GuidStructureStringToGuidString
from Common.BuildToolError import FILE_WRITE_FAILURE
from Common.BuildToolError import CODE_ERROR
from Common.BuildToolError import COMMAND_FAILURE
from Common.BuildToolError import FORMAT_INVALID
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.MultipleWorkspace import MultipleWorkspace as mws
import Common.GlobalData as GlobalData
from AutoGen.ModuleAutoGen import ModuleAutoGen
from Common.Misc import PathClass
from Common.StringUtils import NormPath
from Common.DataType import *
import collections
from Common.Expression import *
from GenFds.AprioriSection import DXE_APRIORI_GUID, PEI_APRIORI_GUID

## Pattern to extract contents in EDK DXS files
gDxsDependencyPattern = re.compile(r"DEPENDENCY_START(.+)DEPENDENCY_END", re.DOTALL)

## Pattern to find total FV total size, occupied size in flash report intermediate file
gFvTotalSizePattern = re.compile(r"EFI_FV_TOTAL_SIZE = (0x[0-9a-fA-F]+)")
gFvTakenSizePattern = re.compile(r"EFI_FV_TAKEN_SIZE = (0x[0-9a-fA-F]+)")

## Pattern to find module size and time stamp in module summary report intermediate file
gModuleSizePattern = re.compile(r"MODULE_SIZE = (\d+)")
gTimeStampPattern  = re.compile(r"TIME_STAMP = (\d+)")

## Pattern to find GUID value in flash description files
gPcdGuidPattern = re.compile(r"PCD\((\w+)[.](\w+)\)")

## Pattern to collect offset, GUID value pair in the flash report intermediate file
gOffsetGuidPattern = re.compile(r"(0x[0-9A-Fa-f]+) ([-A-Fa-f0-9]+)")

## Pattern to find module base address and entry point in fixed flash map file
gModulePattern = r"\n[-\w]+\s*\(([^,]+),\s*BaseAddress=%(Address)s,\s*EntryPoint=%(Address)s,\s*Type=\w+\)\s*\(GUID=([-0-9A-Fa-f]+)[^)]*\)"
gMapFileItemPattern = re.compile(gModulePattern % {"Address" : "(-?0[xX][0-9A-Fa-f]+)"})

## Pattern to find all module referenced header files in source files
gIncludePattern  = re.compile(r'#include\s*["<]([^">]+)[">]')
gIncludePattern2 = re.compile(r"#include\s+EFI_([A-Z_]+)\s*[(]\s*(\w+)\s*[)]")

## Pattern to find the entry point for EDK module using EDKII Glue library
gGlueLibEntryPoint = re.compile(r"__EDKII_GLUE_MODULE_ENTRY_POINT__\s*=\s*(\w+)")

## Tags for MaxLength of line in report
gLineMaxLength = 120

## Tags for end of line in report
gEndOfLine = "\r\n"

## Tags for section start, end and separator
gSectionStart = ">" + "=" * (gLineMaxLength - 2) + "<"
gSectionEnd = "<" + "=" * (gLineMaxLength - 2) + ">" + "\n"
gSectionSep = "=" * gLineMaxLength

## Tags for subsection start, end and separator
gSubSectionStart = ">" + "-" * (gLineMaxLength - 2) + "<"
gSubSectionEnd = "<" + "-" * (gLineMaxLength - 2) + ">"
gSubSectionSep = "-" * gLineMaxLength


## The look up table to map PCD type to pair of report display type and DEC type
gPcdTypeMap = {
  TAB_PCDS_FIXED_AT_BUILD     : ('FIXED',  TAB_PCDS_FIXED_AT_BUILD),
  TAB_PCDS_PATCHABLE_IN_MODULE: ('PATCH',  TAB_PCDS_PATCHABLE_IN_MODULE),
  TAB_PCDS_FEATURE_FLAG       : ('FLAG',   TAB_PCDS_FEATURE_FLAG),
  TAB_PCDS_DYNAMIC            : ('DYN',    TAB_PCDS_DYNAMIC),
  TAB_PCDS_DYNAMIC_HII        : ('DYNHII', TAB_PCDS_DYNAMIC),
  TAB_PCDS_DYNAMIC_VPD        : ('DYNVPD', TAB_PCDS_DYNAMIC),
  TAB_PCDS_DYNAMIC_EX         : ('DEX',    TAB_PCDS_DYNAMIC_EX),
  TAB_PCDS_DYNAMIC_EX_HII     : ('DEXHII', TAB_PCDS_DYNAMIC_EX),
  TAB_PCDS_DYNAMIC_EX_VPD     : ('DEXVPD', TAB_PCDS_DYNAMIC_EX),
  }

## The look up table to map module type to driver type
gDriverTypeMap = {
  SUP_MODULE_SEC               : '0x3 (SECURITY_CORE)',
  SUP_MODULE_PEI_CORE          : '0x4 (PEI_CORE)',
  SUP_MODULE_PEIM              : '0x6 (PEIM)',
  SUP_MODULE_DXE_CORE          : '0x5 (DXE_CORE)',
  SUP_MODULE_DXE_DRIVER        : '0x7 (DRIVER)',
  SUP_MODULE_DXE_SAL_DRIVER    : '0x7 (DRIVER)',
  SUP_MODULE_DXE_SMM_DRIVER    : '0x7 (DRIVER)',
  SUP_MODULE_DXE_RUNTIME_DRIVER: '0x7 (DRIVER)',
  SUP_MODULE_UEFI_DRIVER       : '0x7 (DRIVER)',
  SUP_MODULE_UEFI_APPLICATION  : '0x9 (APPLICATION)',
  SUP_MODULE_SMM_CORE          : '0xD (SMM_CORE)',
  'SMM_DRIVER'        : '0xA (SMM)', # Extension of module type to support PI 1.1 SMM drivers
  SUP_MODULE_MM_STANDALONE     : '0xE (MM_STANDALONE)',
  SUP_MODULE_MM_CORE_STANDALONE : '0xF (MM_CORE_STANDALONE)'
  }

## The look up table of the supported opcode in the dependency expression binaries
gOpCodeList = ["BEFORE", "AFTER", "PUSH", "AND", "OR", "NOT", "TRUE", "FALSE", "END", "SOR"]

## Save VPD Pcd
VPDPcdList = []

##
# Writes a string to the file object.
#
# This function writes a string to the file object and a new line is appended
# afterwards. It may optionally wraps the string for better readability.
#
# @File                      The file object to write
# @String                    The string to be written to the file
# @Wrapper                   Indicates whether to wrap the string
#
def FileWrite(File, String, Wrapper=False):
    if Wrapper:
        String = textwrap.fill(String, 120)
    File.append(String + gEndOfLine)

def ByteArrayForamt(Value):
    IsByteArray = False
    SplitNum = 16
    ArrayList = []
    if Value.startswith('{') and Value.endswith('}') and not Value.startswith("{CODE("):
        Value = Value[1:-1]
        ValueList = Value.split(',')
        if len(ValueList) >= SplitNum:
            IsByteArray = True
    if IsByteArray:
        if ValueList:
            Len = len(ValueList)/SplitNum
            for i, element in enumerate(ValueList):
                ValueList[i] = '0x%02X' % int(element.strip(), 16)
        if Len:
            Id = 0
            while (Id <= Len):
                End = min(SplitNum*(Id+1), len(ValueList))
                Str = ','.join(ValueList[SplitNum*Id : End])
                if End == len(ValueList):
                    Str += '}'
                    ArrayList.append(Str)
                    break
                else:
                    Str += ','
                    ArrayList.append(Str)
                Id += 1
        else:
            ArrayList = [Value + '}']
    return IsByteArray, ArrayList

##
# Find all the header file that the module source directly includes.
#
# This function scans source code to find all header files the module may
# include. This is not accurate but very effective to find all the header
# file the module might include with #include statement.
#
# @Source                    The source file name
# @IncludePathList           The list of include path to find the source file.
# @IncludeFiles              The dictionary of current found include files.
#
def FindIncludeFiles(Source, IncludePathList, IncludeFiles):
    FileContents = open(Source).read()
    #
    # Find header files with pattern #include "XXX.h" or #include <XXX.h>
    #
    for Match in gIncludePattern.finditer(FileContents):
        FileName = Match.group(1).strip()
        for Dir in [os.path.dirname(Source)] + IncludePathList:
            FullFileName = os.path.normpath(os.path.join(Dir, FileName))
            if os.path.exists(FullFileName):
                IncludeFiles[FullFileName.lower().replace("\\", "/")] = FullFileName
                break

    #
    # Find header files with pattern like #include EFI_PPI_CONSUMER(XXX)
    #
    for Match in gIncludePattern2.finditer(FileContents):
        Key = Match.group(2)
        Type = Match.group(1)
        if "ARCH_PROTOCOL" in Type:
            FileName = "ArchProtocol/%(Key)s/%(Key)s.h" % {"Key" : Key}
        elif "PROTOCOL" in Type:
            FileName = "Protocol/%(Key)s/%(Key)s.h" % {"Key" : Key}
        elif "PPI" in Type:
            FileName = "Ppi/%(Key)s/%(Key)s.h" % {"Key" : Key}
        elif TAB_GUID in Type:
            FileName = "Guid/%(Key)s/%(Key)s.h" % {"Key" : Key}
        else:
            continue
        for Dir in IncludePathList:
            FullFileName = os.path.normpath(os.path.join(Dir, FileName))
            if os.path.exists(FullFileName):
                IncludeFiles[FullFileName.lower().replace("\\", "/")] = FullFileName
                break

## Split each lines in file
#
#  This method is used to split the lines in file to make the length of each line
#  less than MaxLength.
#
#  @param      Content           The content of file
#  @param      MaxLength         The Max Length of the line
#
def FileLinesSplit(Content=None, MaxLength=None):
    ContentList = Content.split(TAB_LINE_BREAK)
    NewContent = ''
    NewContentList = []
    for Line in ContentList:
        while len(Line.rstrip()) > MaxLength:
            LineSpaceIndex = Line.rfind(TAB_SPACE_SPLIT, 0, MaxLength)
            LineSlashIndex = Line.rfind(TAB_SLASH, 0, MaxLength)
            LineBackSlashIndex = Line.rfind(TAB_BACK_SLASH, 0, MaxLength)
            if max(LineSpaceIndex, LineSlashIndex, LineBackSlashIndex) > 0:
                LineBreakIndex = max(LineSpaceIndex, LineSlashIndex, LineBackSlashIndex)
            else:
                LineBreakIndex = MaxLength
            NewContentList.append(Line[:LineBreakIndex])
            Line = Line[LineBreakIndex:]
        if Line:
            NewContentList.append(Line)
    for NewLine in NewContentList:
        NewContent += NewLine + TAB_LINE_BREAK

    NewContent = NewContent.replace(gEndOfLine, TAB_LINE_BREAK).replace('\r\r\n', gEndOfLine)
    return NewContent



##
# Parse binary dependency expression section
#
# This utility class parses the dependency expression section and translate the readable
# GUID name and value.
#
class DepexParser(object):
    ##
    # Constructor function for class DepexParser
    #
    # This constructor function collect GUID values so that the readable
    # GUID name can be translated.
    #
    # @param self            The object pointer
    # @param Wa              Workspace context information
    #
    def __init__(self, Wa):
        self._GuidDb = {}
        for Pa in Wa.AutoGenObjectList:
            for Package in Pa.PackageList:
                for Protocol in Package.Protocols:
                    GuidValue = GuidStructureStringToGuidString(Package.Protocols[Protocol])
                    self._GuidDb[GuidValue.upper()] = Protocol
                for Ppi in Package.Ppis:
                    GuidValue = GuidStructureStringToGuidString(Package.Ppis[Ppi])
                    self._GuidDb[GuidValue.upper()] = Ppi
                for Guid in Package.Guids:
                    GuidValue = GuidStructureStringToGuidString(Package.Guids[Guid])
                    self._GuidDb[GuidValue.upper()] = Guid
            for Ma in Pa.ModuleAutoGenList:
                for Pcd in Ma.FixedVoidTypePcds:
                    PcdValue = Ma.FixedVoidTypePcds[Pcd]
                    if len(PcdValue.split(',')) == 16:
                        GuidValue = GuidStructureByteArrayToGuidString(PcdValue)
                        self._GuidDb[GuidValue.upper()] = Pcd
    ##
    # Parse the binary dependency expression files.
    #
    # This function parses the binary dependency expression file and translate it
    # to the instruction list.
    #
    # @param self            The object pointer
    # @param DepexFileName   The file name of binary dependency expression file.
    #
    def ParseDepexFile(self, DepexFileName):
        DepexFile = open(DepexFileName, "rb")
        DepexStatement = []
        OpCode = DepexFile.read(1)
        while OpCode:
            Statement = gOpCodeList[struct.unpack("B", OpCode)[0]]
            if Statement in ["BEFORE", "AFTER", "PUSH"]:
                GuidValue = "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X" % \
                            struct.unpack(PACK_PATTERN_GUID, DepexFile.read(16))
                GuidString = self._GuidDb.get(GuidValue, GuidValue)
                Statement = "%s %s" % (Statement, GuidString)
            DepexStatement.append(Statement)
            OpCode = DepexFile.read(1)

        return DepexStatement

##
# Reports library information
#
# This class reports the module library subsection in the build report file.
#
class LibraryReport(object):
    ##
    # Constructor function for class LibraryReport
    #
    # This constructor function generates LibraryReport object for
    # a module.
    #
    # @param self            The object pointer
    # @param M               Module context information
    #
    def __init__(self, M):
        self.LibraryList = []

        for Lib in M.DependentLibraryList:
            LibInfPath = str(Lib)
            LibClassList = Lib.LibraryClass[0].LibraryClass
            LibConstructorList = Lib.ConstructorList
            LibDesstructorList = Lib.DestructorList
            LibDepexList = Lib.DepexExpression[M.Arch, M.ModuleType]
            for LibAutoGen in M.LibraryAutoGenList:
                if LibInfPath == LibAutoGen.MetaFile.Path:
                    LibTime = LibAutoGen.BuildTime
                    break
            self.LibraryList.append((LibInfPath, LibClassList, LibConstructorList, LibDesstructorList, LibDepexList, LibTime))

    ##
    # Generate report for module library information
    #
    # This function generates report for the module library.
    # If the module is EDKII style one, the additional library class, library
    # constructor/destructor and dependency expression may also be reported.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    #
    def GenerateReport(self, File):
        if len(self.LibraryList) > 0:
            FileWrite(File, gSubSectionStart)
            FileWrite(File, TAB_BRG_LIBRARY)
            FileWrite(File, gSubSectionSep)
            for LibraryItem in self.LibraryList:
                LibInfPath = LibraryItem[0]
                FileWrite(File, LibInfPath)

                LibClass = LibraryItem[1]
                EdkIILibInfo = ""
                LibConstructor = " ".join(LibraryItem[2])
                if LibConstructor:
                    EdkIILibInfo += " C = " + LibConstructor
                LibDestructor = " ".join(LibraryItem[3])
                if LibDestructor:
                    EdkIILibInfo += " D = " + LibDestructor
                LibDepex = " ".join(LibraryItem[4])
                if LibDepex:
                    EdkIILibInfo += " Depex = " + LibDepex
                if LibraryItem[5]:
                    EdkIILibInfo += " Time = " + LibraryItem[5]
                if EdkIILibInfo:
                    FileWrite(File, "{%s: %s}" % (LibClass, EdkIILibInfo))
                else:
                    FileWrite(File, "{%s}" % LibClass)

            FileWrite(File, gSubSectionEnd)

##
# Reports dependency expression information
#
# This class reports the module dependency expression subsection in the build report file.
#
class DepexReport(object):
    ##
    # Constructor function for class DepexReport
    #
    # This constructor function generates DepexReport object for
    # a module. If the module source contains the DXS file (usually EDK
    # style module), it uses the dependency in DXS file; otherwise,
    # it uses the dependency expression from its own INF [Depex] section
    # and then merges with the ones from its dependent library INF.
    #
    # @param self            The object pointer
    # @param M               Module context information
    #
    def __init__(self, M):
        self.Depex = ""
        self._DepexFileName = os.path.join(M.BuildDir, "OUTPUT", M.Module.BaseName + ".depex")
        ModuleType = M.ModuleType
        if not ModuleType:
            ModuleType = COMPONENT_TO_MODULE_MAP_DICT.get(M.ComponentType, "")

        if ModuleType in [SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_DXE_CORE, SUP_MODULE_SMM_CORE, SUP_MODULE_MM_CORE_STANDALONE, SUP_MODULE_UEFI_APPLICATION]:
            return

        for Source in M.SourceFileList:
            if os.path.splitext(Source.Path)[1].lower() == ".dxs":
                Match = gDxsDependencyPattern.search(open(Source.Path).read())
                if Match:
                    self.Depex = Match.group(1).strip()
                    self.Source = "DXS"
                    break
        else:
            self.Depex = M.DepexExpressionDict.get(M.ModuleType, "")
            self.ModuleDepex = " ".join(M.Module.DepexExpression[M.Arch, M.ModuleType])
            if not self.ModuleDepex:
                self.ModuleDepex = "(None)"

            LibDepexList = []
            for Lib in M.DependentLibraryList:
                LibDepex = " ".join(Lib.DepexExpression[M.Arch, M.ModuleType]).strip()
                if LibDepex != "":
                    LibDepexList.append("(" + LibDepex + ")")
            self.LibraryDepex = " AND ".join(LibDepexList)
            if not self.LibraryDepex:
                self.LibraryDepex = "(None)"
            self.Source = "INF"

    ##
    # Generate report for module dependency expression information
    #
    # This function generates report for the module dependency expression.
    #
    # @param self              The object pointer
    # @param File              The file object for report
    # @param GlobalDepexParser The platform global Dependency expression parser object
    #
    def GenerateReport(self, File, GlobalDepexParser):
        if not self.Depex:
            return
        FileWrite(File, gSubSectionStart)
        if os.path.isfile(self._DepexFileName):
            try:
                DepexStatements = GlobalDepexParser.ParseDepexFile(self._DepexFileName)
                FileWrite(File, "Final Dependency Expression (DEPEX) Instructions")
                for DepexStatement in DepexStatements:
                    FileWrite(File, "  %s" % DepexStatement)
                FileWrite(File, gSubSectionSep)
            except:
                EdkLogger.warn(None, "Dependency expression file is corrupted", self._DepexFileName)

        FileWrite(File, "Dependency Expression (DEPEX) from %s" % self.Source)

        if self.Source == "INF":
            FileWrite(File, self.Depex, True)
            FileWrite(File, gSubSectionSep)
            FileWrite(File, "From Module INF:  %s" % self.ModuleDepex, True)
            FileWrite(File, "From Library INF: %s" % self.LibraryDepex, True)
        else:
            FileWrite(File, self.Depex)
        FileWrite(File, gSubSectionEnd)

##
# Reports dependency expression information
#
# This class reports the module build flags subsection in the build report file.
#
class BuildFlagsReport(object):
    ##
    # Constructor function for class BuildFlagsReport
    #
    # This constructor function generates BuildFlagsReport object for
    # a module. It reports the build tool chain tag and all relevant
    # build flags to build the module.
    #
    # @param self            The object pointer
    # @param M               Module context information
    #
    def __init__(self, M):
        BuildOptions = {}
        #
        # Add build flags according to source file extension so that
        # irrelevant ones can be filtered out.
        #
        for Source in M.SourceFileList:
            Ext = os.path.splitext(Source.File)[1].lower()
            if Ext in [".c", ".cc", ".cpp"]:
                BuildOptions["CC"] = 1
            elif Ext in [".s", ".asm"]:
                BuildOptions["PP"] = 1
                BuildOptions["ASM"] = 1
            elif Ext in [".vfr"]:
                BuildOptions["VFRPP"] = 1
                BuildOptions["VFR"] = 1
            elif Ext in [".dxs"]:
                BuildOptions["APP"] = 1
                BuildOptions["CC"] = 1
            elif Ext in [".asl"]:
                BuildOptions["ASLPP"] = 1
                BuildOptions["ASL"] = 1
            elif Ext in [".aslc"]:
                BuildOptions["ASLCC"] = 1
                BuildOptions["ASLDLINK"] = 1
                BuildOptions["CC"] = 1
            elif Ext in [".asm16"]:
                BuildOptions["ASMLINK"] = 1
            BuildOptions["SLINK"] = 1
            BuildOptions["DLINK"] = 1

        #
        # Save module build flags.
        #
        self.ToolChainTag = M.ToolChain
        self.BuildFlags = {}
        for Tool in BuildOptions:
            self.BuildFlags[Tool + "_FLAGS"] = M.BuildOption.get(Tool, {}).get("FLAGS", "")

    ##
    # Generate report for module build flags information
    #
    # This function generates report for the module build flags expression.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    #
    def GenerateReport(self, File):
        FileWrite(File, gSubSectionStart)
        FileWrite(File, "Build Flags")
        FileWrite(File, "Tool Chain Tag: %s" % self.ToolChainTag)
        for Tool in self.BuildFlags:
            FileWrite(File, gSubSectionSep)
            FileWrite(File, "%s = %s" % (Tool, self.BuildFlags[Tool]), True)

        FileWrite(File, gSubSectionEnd)


##
# Reports individual module information
#
# This class reports the module section in the build report file.
# It comprises of module summary, module PCD, library, dependency expression,
# build flags sections.
#
class ModuleReport(object):
    ##
    # Constructor function for class ModuleReport
    #
    # This constructor function generates ModuleReport object for
    # a separate module in a platform build.
    #
    # @param self            The object pointer
    # @param M               Module context information
    # @param ReportType      The kind of report items in the final report file
    #
    def __init__(self, M, ReportType):
        self.ModuleName = M.Module.BaseName
        self.ModuleInfPath = M.MetaFile.File
        self.ModuleArch = M.Arch
        self.FileGuid = M.Guid
        self.Size = 0
        self.BuildTimeStamp = None
        self.Hash = 0
        self.DriverType = ""
        if not M.IsLibrary:
            ModuleType = M.ModuleType
            if not ModuleType:
                ModuleType = COMPONENT_TO_MODULE_MAP_DICT.get(M.ComponentType, "")
            #
            # If a module complies to PI 1.1, promote Module type to "SMM_DRIVER"
            #
            if ModuleType == SUP_MODULE_DXE_SMM_DRIVER:
                PiSpec = M.Module.Specification.get("PI_SPECIFICATION_VERSION", "0x00010000")
                if int(PiSpec, 0) >= 0x0001000A:
                    ModuleType = "SMM_DRIVER"
            self.DriverType = gDriverTypeMap.get(ModuleType, "0x2 (FREE_FORM)")
        self.UefiSpecVersion = M.Module.Specification.get("UEFI_SPECIFICATION_VERSION", "")
        self.PiSpecVersion = M.Module.Specification.get("PI_SPECIFICATION_VERSION", "")
        self.PciDeviceId = M.Module.Defines.get("PCI_DEVICE_ID", "")
        self.PciVendorId = M.Module.Defines.get("PCI_VENDOR_ID", "")
        self.PciClassCode = M.Module.Defines.get("PCI_CLASS_CODE", "")
        self.BuildTime = M.BuildTime

        self._BuildDir = M.BuildDir
        self.ModulePcdSet = {}
        if "PCD" in ReportType:
            #
            # Collect all module used PCD set: module INF referenced directly or indirectly.
            # It also saves module INF default values of them in case they exist.
            #
            for Pcd in M.ModulePcdList + M.LibraryPcdList:
                self.ModulePcdSet.setdefault((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Type), (Pcd.InfDefaultValue, Pcd.DefaultValue))

        self.LibraryReport = None
        if "LIBRARY" in ReportType:
            self.LibraryReport = LibraryReport(M)

        self.DepexReport = None
        if "DEPEX" in ReportType:
            self.DepexReport = DepexReport(M)

        if "BUILD_FLAGS" in ReportType:
            self.BuildFlagsReport = BuildFlagsReport(M)


    ##
    # Generate report for module information
    #
    # This function generates report for separate module expression
    # in a platform build.
    #
    # @param self                   The object pointer
    # @param File                   The file object for report
    # @param GlobalPcdReport        The platform global PCD report object
    # @param GlobalPredictionReport The platform global Prediction report object
    # @param GlobalDepexParser      The platform global Dependency expression parser object
    # @param ReportType             The kind of report items in the final report file
    #
    def GenerateReport(self, File, GlobalPcdReport, GlobalPredictionReport, GlobalDepexParser, ReportType):
        FileWrite(File, gSectionStart)

        FwReportFileName = os.path.join(self._BuildDir, "OUTPUT", self.ModuleName + ".txt")
        if os.path.isfile(FwReportFileName):
            try:
                FileContents = open(FwReportFileName).read()
                Match = gModuleSizePattern.search(FileContents)
                if Match:
                    self.Size = int(Match.group(1))

                Match = gTimeStampPattern.search(FileContents)
                if Match:
                    self.BuildTimeStamp = datetime.utcfromtimestamp(int(Match.group(1)))
            except IOError:
                EdkLogger.warn(None, "Fail to read report file", FwReportFileName)

        if "HASH" in ReportType:
            OutputDir = os.path.join(self._BuildDir, "OUTPUT")
            DefaultEFIfile = os.path.join(OutputDir, self.ModuleName + ".efi")
            if os.path.isfile(DefaultEFIfile):
                Tempfile = os.path.join(OutputDir, self.ModuleName + "_hash.tmp")
                # rebase the efi image since its base address may not zero
                cmd = ["GenFw", "--rebase", str(0), "-o", Tempfile, DefaultEFIfile]
                try:
                    PopenObject = subprocess.Popen(' '.join(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                except Exception as X:
                    EdkLogger.error("GenFw", COMMAND_FAILURE, ExtraData="%s: %s" % (str(X), cmd[0]))
                EndOfProcedure = threading.Event()
                EndOfProcedure.clear()
                if PopenObject.stderr:
                    StdErrThread = threading.Thread(target=ReadMessage, args=(PopenObject.stderr, EdkLogger.quiet, EndOfProcedure))
                    StdErrThread.setName("STDERR-Redirector")
                    StdErrThread.setDaemon(False)
                    StdErrThread.start()
                # waiting for program exit
                PopenObject.wait()
                if PopenObject.stderr:
                    StdErrThread.join()
                if PopenObject.returncode != 0:
                    EdkLogger.error("GenFw", COMMAND_FAILURE, "Failed to generate firmware hash image for %s" % (DefaultEFIfile))
                if os.path.isfile(Tempfile):
                    self.Hash = hashlib.sha1()
                    buf = open(Tempfile, 'rb').read()
                    if self.Hash.update(buf):
                        self.Hash = self.Hash.update(buf)
                    self.Hash = self.Hash.hexdigest()
                    os.remove(Tempfile)

        FileWrite(File, "Module Summary")
        FileWrite(File, "Module Name:          %s" % self.ModuleName)
        FileWrite(File, "Module Arch:          %s" % self.ModuleArch)
        FileWrite(File, "Module INF Path:      %s" % self.ModuleInfPath)
        FileWrite(File, "File GUID:            %s" % self.FileGuid)
        if self.Size:
            FileWrite(File, "Size:                 0x%X (%.2fK)" % (self.Size, self.Size / 1024.0))
        if self.Hash:
            FileWrite(File, "SHA1 HASH:            %s *%s" % (self.Hash, self.ModuleName + ".efi"))
        if self.BuildTimeStamp:
            FileWrite(File, "Build Time Stamp:     %s" % self.BuildTimeStamp)
        if self.BuildTime:
            FileWrite(File, "Module Build Time:    %s" % self.BuildTime)
        if self.DriverType:
            FileWrite(File, "Driver Type:          %s" % self.DriverType)
        if self.UefiSpecVersion:
            FileWrite(File, "UEFI Spec Version:    %s" % self.UefiSpecVersion)
        if self.PiSpecVersion:
            FileWrite(File, "PI Spec Version:      %s" % self.PiSpecVersion)
        if self.PciDeviceId:
            FileWrite(File, "PCI Device ID:        %s" % self.PciDeviceId)
        if self.PciVendorId:
            FileWrite(File, "PCI Vendor ID:        %s" % self.PciVendorId)
        if self.PciClassCode:
            FileWrite(File, "PCI Class Code:       %s" % self.PciClassCode)

        FileWrite(File, gSectionSep)

        if "PCD" in ReportType:
            GlobalPcdReport.GenerateReport(File, self.ModulePcdSet,self.FileGuid)

        if "LIBRARY" in ReportType:
            self.LibraryReport.GenerateReport(File)

        if "DEPEX" in ReportType:
            self.DepexReport.GenerateReport(File, GlobalDepexParser)

        if "BUILD_FLAGS" in ReportType:
            self.BuildFlagsReport.GenerateReport(File)

        if "FIXED_ADDRESS" in ReportType and self.FileGuid:
            GlobalPredictionReport.GenerateReport(File, self.FileGuid)

        FileWrite(File, gSectionEnd)

def ReadMessage(From, To, ExitFlag):
    while True:
        # read one line a time
        Line = From.readline()
        # empty string means "end"
        if Line is not None and Line != b"":
            To(Line.rstrip().decode(encoding='utf-8', errors='ignore'))
        else:
            break
        if ExitFlag.isSet():
            break

##
# Reports platform and module PCD information
#
# This class reports the platform PCD section and module PCD subsection
# in the build report file.
#
class PcdReport(object):
    ##
    # Constructor function for class PcdReport
    #
    # This constructor function generates PcdReport object a platform build.
    # It collects the whole PCD database from platform DSC files, platform
    # flash description file and package DEC files.
    #
    # @param self            The object pointer
    # @param Wa              Workspace context information
    #
    def __init__(self, Wa):
        self.AllPcds = {}
        self.UnusedPcds = {}
        self.ConditionalPcds = {}
        self.MaxLen = 0
        self.Arch = None
        if Wa.FdfProfile:
            self.FdfPcdSet = Wa.FdfProfile.PcdDict
        else:
            self.FdfPcdSet = {}

        self.DefaultStoreSingle = True
        self.SkuSingle = True
        if GlobalData.gDefaultStores and len(GlobalData.gDefaultStores) > 1:
            self.DefaultStoreSingle = False
        if GlobalData.gSkuids and len(GlobalData.gSkuids) > 1:
            self.SkuSingle = False

        self.ModulePcdOverride = {}
        for Pa in Wa.AutoGenObjectList:
            self.Arch = Pa.Arch
            #
            # Collect all platform referenced PCDs and grouped them by PCD token space
            # GUID C Names
            #
            for Pcd in Pa.AllPcdList:
                PcdList = self.AllPcds.setdefault(Pcd.TokenSpaceGuidCName, {}).setdefault(Pcd.Type, [])
                if Pcd not in PcdList:
                    PcdList.append(Pcd)
                if len(Pcd.TokenCName) > self.MaxLen:
                    self.MaxLen = len(Pcd.TokenCName)
            #
            # Collect the PCD defined in DSC/FDF file, but not used in module
            #
            UnusedPcdFullList = []
            StructPcdDict = GlobalData.gStructurePcd.get(self.Arch, collections.OrderedDict())
            for Name, Guid in StructPcdDict:
                if (Name, Guid) not in Pa.Platform.Pcds:
                    Pcd = StructPcdDict[(Name, Guid)]
                    PcdList = self.AllPcds.setdefault(Guid, {}).setdefault(Pcd.Type, [])
                    if Pcd not in PcdList and Pcd not in UnusedPcdFullList:
                        UnusedPcdFullList.append(Pcd)
            for item in Pa.Platform.Pcds:
                Pcd = Pa.Platform.Pcds[item]
                if not Pcd.Type:
                    # check the Pcd in FDF file, whether it is used in module first
                    for T in PCD_TYPE_LIST:
                        PcdList = self.AllPcds.setdefault(Pcd.TokenSpaceGuidCName, {}).setdefault(T, [])
                        if Pcd in PcdList:
                            Pcd.Type = T
                            break
                if not Pcd.Type:
                    PcdTypeFlag = False
                    for package in Pa.PackageList:
                        for T in PCD_TYPE_LIST:
                            if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, T) in package.Pcds:
                                Pcd.Type = T
                                PcdTypeFlag = True
                                if not Pcd.DatumType:
                                    Pcd.DatumType = package.Pcds[(Pcd.TokenCName, Pcd.TokenSpaceGuidCName, T)].DatumType
                                break
                        if PcdTypeFlag:
                            break
                if not Pcd.DatumType:
                    PcdType = Pcd.Type
                    # Try to remove Hii and Vpd suffix
                    if PcdType.startswith(TAB_PCDS_DYNAMIC_EX):
                        PcdType = TAB_PCDS_DYNAMIC_EX
                    elif PcdType.startswith(TAB_PCDS_DYNAMIC):
                        PcdType = TAB_PCDS_DYNAMIC
                    for package in Pa.PackageList:
                        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, PcdType) in package.Pcds:
                            Pcd.DatumType = package.Pcds[(Pcd.TokenCName, Pcd.TokenSpaceGuidCName, PcdType)].DatumType
                            break

                PcdList = self.AllPcds.setdefault(Pcd.TokenSpaceGuidCName, {}).setdefault(Pcd.Type, [])
                UnusedPcdList = self.UnusedPcds.setdefault(Pcd.TokenSpaceGuidCName, {}).setdefault(Pcd.Type, [])
                if Pcd in UnusedPcdList:
                    UnusedPcdList.remove(Pcd)
                if Pcd not in PcdList and Pcd not in UnusedPcdFullList:
                    UnusedPcdFullList.append(Pcd)
                if len(Pcd.TokenCName) > self.MaxLen:
                    self.MaxLen = len(Pcd.TokenCName)

            if GlobalData.gConditionalPcds:
                for PcdItem in GlobalData.gConditionalPcds:
                    if '.' in PcdItem:
                        (TokenSpaceGuidCName, TokenCName) = PcdItem.split('.')
                        if (TokenCName, TokenSpaceGuidCName) in Pa.Platform.Pcds:
                            Pcd = Pa.Platform.Pcds[(TokenCName, TokenSpaceGuidCName)]
                            PcdList = self.ConditionalPcds.setdefault(Pcd.TokenSpaceGuidCName, {}).setdefault(Pcd.Type, [])
                            if Pcd not in PcdList:
                                PcdList.append(Pcd)

            UnusedPcdList = []
            if UnusedPcdFullList:
                for Pcd in UnusedPcdFullList:
                    if Pcd.TokenSpaceGuidCName + '.' + Pcd.TokenCName in GlobalData.gConditionalPcds:
                        continue
                    UnusedPcdList.append(Pcd)

            for Pcd in UnusedPcdList:
                PcdList = self.UnusedPcds.setdefault(Pcd.TokenSpaceGuidCName, {}).setdefault(Pcd.Type, [])
                if Pcd not in PcdList:
                    PcdList.append(Pcd)

            for Module in Pa.Platform.Modules.values():
                #
                # Collect module override PCDs
                #
                for ModulePcd in Module.M.ModulePcdList + Module.M.LibraryPcdList:
                    TokenCName = ModulePcd.TokenCName
                    TokenSpaceGuid = ModulePcd.TokenSpaceGuidCName
                    ModuleDefault = ModulePcd.DefaultValue
                    ModulePath = os.path.basename(Module.M.MetaFile.File)
                    self.ModulePcdOverride.setdefault((TokenCName, TokenSpaceGuid), {})[ModulePath] = ModuleDefault


        #
        # Collect PCD DEC default value.
        #
        self.DecPcdDefault = {}
        self._GuidDict = {}
        for Pa in Wa.AutoGenObjectList:
            for Package in Pa.PackageList:
                Guids = Package.Guids
                self._GuidDict.update(Guids)
                for (TokenCName, TokenSpaceGuidCName, DecType) in Package.Pcds:
                    DecDefaultValue = Package.Pcds[TokenCName, TokenSpaceGuidCName, DecType].DefaultValue
                    self.DecPcdDefault.setdefault((TokenCName, TokenSpaceGuidCName, DecType), DecDefaultValue)
        #
        # Collect PCDs defined in DSC common section
        #
        self.DscPcdDefault = {}
        for Pa in Wa.AutoGenObjectList:
            for (TokenCName, TokenSpaceGuidCName) in Pa.Platform.Pcds:
                DscDefaultValue = Pa.Platform.Pcds[(TokenCName, TokenSpaceGuidCName)].DscDefaultValue
                if DscDefaultValue:
                    self.DscPcdDefault[(TokenCName, TokenSpaceGuidCName)] = DscDefaultValue

    def GenerateReport(self, File, ModulePcdSet,ModuleGuid=None):
        if not ModulePcdSet:
            if self.ConditionalPcds:
                self.GenerateReportDetail(File, ModulePcdSet, 1)
            if self.UnusedPcds:
                IsEmpty = True
                for Token in self.UnusedPcds:
                    TokenDict = self.UnusedPcds[Token]
                    for Type in TokenDict:
                        if TokenDict[Type]:
                            IsEmpty = False
                            break
                    if not IsEmpty:
                        break
                if not IsEmpty:
                    self.GenerateReportDetail(File, ModulePcdSet, 2)
        self.GenerateReportDetail(File, ModulePcdSet,ModuleGuid = ModuleGuid)

    ##
    # Generate report for PCD information
    #
    # This function generates report for separate module expression
    # in a platform build.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    # @param ModulePcdSet    Set of all PCDs referenced by module or None for
    #                        platform PCD report
    # @param ReportySubType  0 means platform/module PCD report, 1 means Conditional
    #                        directives section report, 2 means Unused Pcds section report
    # @param DscOverridePcds Module DSC override PCDs set
    #
    def GenerateReportDetail(self, File, ModulePcdSet, ReportSubType = 0,ModuleGuid=None):
        PcdDict = self.AllPcds
        if ReportSubType == 1:
            PcdDict = self.ConditionalPcds
        elif ReportSubType == 2:
            PcdDict = self.UnusedPcds

        if not ModulePcdSet:
            FileWrite(File, gSectionStart)
            if ReportSubType == 1:
                FileWrite(File, "Conditional Directives used by the build system")
            elif ReportSubType == 2:
                FileWrite(File, "PCDs not used by modules or in conditional directives")
            else:
                FileWrite(File, "Platform Configuration Database Report")

            FileWrite(File, "  *B  - PCD override in the build option")
            FileWrite(File, "  *P  - Platform scoped PCD override in DSC file")
            FileWrite(File, "  *F  - Platform scoped PCD override in FDF file")
            if not ReportSubType:
                FileWrite(File, "  *M  - Module scoped PCD override")
            FileWrite(File, gSectionSep)
        else:
            if not ReportSubType and ModulePcdSet:
                #
                # For module PCD sub-section
                #
                FileWrite(File, gSubSectionStart)
                FileWrite(File, TAB_BRG_PCD)
                FileWrite(File, gSubSectionSep)
        AllPcdDict = {}
        for Key in PcdDict:
            AllPcdDict[Key] = {}
            for Type in PcdDict[Key]:
                for Pcd in PcdDict[Key][Type]:
                    AllPcdDict[Key][(Pcd.TokenCName, Type)] = Pcd
        for Key in sorted(AllPcdDict):
            #
            # Group PCD by their token space GUID C Name
            #
            First = True
            for PcdTokenCName, Type in sorted(AllPcdDict[Key]):
                #
                # Group PCD by their usage type
                #
                Pcd = AllPcdDict[Key][(PcdTokenCName, Type)]
                TypeName, DecType = gPcdTypeMap.get(Type, ("", Type))
                MixedPcdFlag = False
                if GlobalData.MixedPcd:
                    for PcdKey in GlobalData.MixedPcd:
                        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdKey]:
                            PcdTokenCName = PcdKey[0]
                            MixedPcdFlag = True
                    if MixedPcdFlag and not ModulePcdSet:
                        continue
                #
                # Get PCD default value and their override relationship
                #
                DecDefaultValue = self.DecPcdDefault.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, DecType))
                DscDefaultValue = self.DscPcdDefault.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName))
                DscDefaultValBak = DscDefaultValue
                Field = ''
                for (CName, Guid, Field) in self.FdfPcdSet:
                    if CName == PcdTokenCName and Guid == Key:
                        DscDefaultValue = self.FdfPcdSet[(CName, Guid, Field)]
                        break
                if DscDefaultValue != DscDefaultValBak:
                    try:
                        DscDefaultValue = ValueExpressionEx(DscDefaultValue, Pcd.DatumType, self._GuidDict)(True)
                    except BadExpression as DscDefaultValue:
                        EdkLogger.error('BuildReport', FORMAT_INVALID, "PCD Value: %s, Type: %s" %(DscDefaultValue, Pcd.DatumType))

                InfDefaultValue = None

                PcdValue = DecDefaultValue
                if DscDefaultValue:
                    PcdValue = DscDefaultValue
                #The DefaultValue of StructurePcd already be the latest, no need to update.
                if not self.IsStructurePcd(Pcd.TokenCName, Pcd.TokenSpaceGuidCName):
                    Pcd.DefaultValue = PcdValue
                PcdComponentValue = None
                if ModulePcdSet is not None:
                    if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Type) not in ModulePcdSet:
                        continue
                    InfDefaultValue, PcdComponentValue = ModulePcdSet[Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Type]
                    PcdValue = PcdComponentValue
                    #The DefaultValue of StructurePcd already be the latest, no need to update.
                    if not self.IsStructurePcd(Pcd.TokenCName, Pcd.TokenSpaceGuidCName):
                        Pcd.DefaultValue = PcdValue
                    if InfDefaultValue:
                        try:
                            InfDefaultValue = ValueExpressionEx(InfDefaultValue, Pcd.DatumType, self._GuidDict)(True)
                        except BadExpression as InfDefaultValue:
                            EdkLogger.error('BuildReport', FORMAT_INVALID, "PCD Value: %s, Type: %s" % (InfDefaultValue, Pcd.DatumType))
                    if InfDefaultValue == "":
                        InfDefaultValue = None

                BuildOptionMatch = False
                if GlobalData.BuildOptionPcd:
                    for pcd in GlobalData.BuildOptionPcd:
                        if (Pcd.TokenSpaceGuidCName, Pcd.TokenCName) == (pcd[0], pcd[1]):
                            if pcd[2]:
                                continue
                            PcdValue = pcd[3]
                            #The DefaultValue of StructurePcd already be the latest, no need to update.
                            if not self.IsStructurePcd(Pcd.TokenCName, Pcd.TokenSpaceGuidCName):
                                Pcd.DefaultValue = PcdValue
                            BuildOptionMatch = True
                            break

                if First:
                    if ModulePcdSet is None:
                        FileWrite(File, "")
                    FileWrite(File, Key)
                    First = False


                if Pcd.DatumType in TAB_PCD_NUMERIC_TYPES:
                    if PcdValue.startswith('0') and not PcdValue.lower().startswith('0x') and \
                            len(PcdValue) > 1 and PcdValue.lstrip('0'):
                        PcdValue = PcdValue.lstrip('0')
                    PcdValueNumber = int(PcdValue.strip(), 0)
                    if DecDefaultValue is None:
                        DecMatch = True
                    else:
                        if DecDefaultValue.startswith('0') and not DecDefaultValue.lower().startswith('0x') and \
                                len(DecDefaultValue) > 1 and DecDefaultValue.lstrip('0'):
                            DecDefaultValue = DecDefaultValue.lstrip('0')
                        DecDefaultValueNumber = int(DecDefaultValue.strip(), 0)
                        DecMatch = (DecDefaultValueNumber == PcdValueNumber)

                    if InfDefaultValue is None:
                        InfMatch = True
                    else:
                        if InfDefaultValue.startswith('0') and not InfDefaultValue.lower().startswith('0x') and \
                                len(InfDefaultValue) > 1 and InfDefaultValue.lstrip('0'):
                            InfDefaultValue = InfDefaultValue.lstrip('0')
                        InfDefaultValueNumber = int(InfDefaultValue.strip(), 0)
                        InfMatch = (InfDefaultValueNumber == PcdValueNumber)

                    if DscDefaultValue is None:
                        DscMatch = True
                    else:
                        if DscDefaultValue.startswith('0') and not DscDefaultValue.lower().startswith('0x') and \
                                len(DscDefaultValue) > 1 and DscDefaultValue.lstrip('0'):
                            DscDefaultValue = DscDefaultValue.lstrip('0')
                        DscDefaultValueNumber = int(DscDefaultValue.strip(), 0)
                        DscMatch = (DscDefaultValueNumber == PcdValueNumber)
                else:
                    if DecDefaultValue is None:
                        DecMatch = True
                    else:
                        DecMatch = (DecDefaultValue.strip() == PcdValue.strip())

                    if InfDefaultValue is None:
                        InfMatch = True
                    else:
                        InfMatch = (InfDefaultValue.strip() == PcdValue.strip())

                    if DscDefaultValue is None:
                        DscMatch = True
                    else:
                        DscMatch = (DscDefaultValue.strip() == PcdValue.strip())

                IsStructure = False
                if self.IsStructurePcd(Pcd.TokenCName, Pcd.TokenSpaceGuidCName):
                    IsStructure = True
                    if TypeName in ('DYNVPD', 'DEXVPD'):
                        SkuInfoList = Pcd.SkuInfoList
                    Pcd = GlobalData.gStructurePcd[self.Arch][(Pcd.TokenCName, Pcd.TokenSpaceGuidCName)]
                    if ModulePcdSet and ModulePcdSet.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Type)):
                        InfDefaultValue, PcdComponentValue = ModulePcdSet[Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Type]
                        DscDefaultValBak = Pcd.DefaultValue
                        Pcd.DefaultValue = PcdComponentValue

                    Pcd.DatumType = Pcd.StructName
                    if TypeName in ('DYNVPD', 'DEXVPD'):
                        Pcd.SkuInfoList = SkuInfoList
                    if Pcd.PcdValueFromComm or Pcd.PcdFieldValueFromComm:
                        BuildOptionMatch = True
                        DecMatch = False
                    elif Pcd.PcdValueFromFdf or Pcd.PcdFieldValueFromFdf:
                        DscDefaultValue = True
                        DscMatch = True
                        DecMatch = False
                    else:
                        if Pcd.Type in PCD_DYNAMIC_TYPE_SET | PCD_DYNAMIC_EX_TYPE_SET:
                            DscOverride = False
                            if Pcd.DefaultFromDSC:
                                DscOverride = True
                            else:
                                DictLen = 0
                                for item in Pcd.SkuOverrideValues:
                                    DictLen += len(Pcd.SkuOverrideValues[item])
                                if not DictLen:
                                    DscOverride = False
                                else:
                                    if not Pcd.SkuInfoList:
                                        OverrideValues = Pcd.SkuOverrideValues
                                        if OverrideValues:
                                            for Data in OverrideValues.values():
                                                Struct = list(Data.values())
                                                if Struct:
                                                    DscOverride = self.ParseStruct(Struct[0])
                                                    break
                                    else:
                                        SkuList = sorted(Pcd.SkuInfoList.keys())
                                        for Sku in SkuList:
                                            SkuInfo = Pcd.SkuInfoList[Sku]
                                            if SkuInfo.DefaultStoreDict:
                                                DefaultStoreList = sorted(SkuInfo.DefaultStoreDict.keys())
                                                for DefaultStore in DefaultStoreList:
                                                    OverrideValues = Pcd.SkuOverrideValues.get(Sku)
                                                    if OverrideValues:
                                                        DscOverride = self.ParseStruct(OverrideValues[DefaultStore])
                                                        if DscOverride:
                                                            break
                                            if DscOverride:
                                                break
                            if DscOverride:
                                DscDefaultValue = True
                                DscMatch = True
                                DecMatch = False
                            else:
                                DecMatch = True
                        else:
                            if Pcd.DscRawValue or (ModuleGuid and ModuleGuid.replace("-","S") in Pcd.PcdValueFromComponents):
                                DscDefaultValue = True
                                DscMatch = True
                                DecMatch = False
                            else:
                                DscDefaultValue = False
                                DecMatch = True

                #
                # Report PCD item according to their override relationship
                #
                if Pcd.DatumType == 'BOOLEAN':
                    if DscDefaultValue:
                        DscDefaultValue = str(int(DscDefaultValue, 0))
                    if DecDefaultValue:
                        DecDefaultValue = str(int(DecDefaultValue, 0))
                    if InfDefaultValue:
                        InfDefaultValue = str(int(InfDefaultValue, 0))
                    if Pcd.DefaultValue:
                        Pcd.DefaultValue = str(int(Pcd.DefaultValue, 0))
                if DecMatch:
                    self.PrintPcdValue(File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValBak, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue, '  ')
                elif InfDefaultValue and InfMatch:
                    self.PrintPcdValue(File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValBak, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue, '*M')
                elif BuildOptionMatch:
                    self.PrintPcdValue(File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValBak, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue, '*B')
                else:
                    if PcdComponentValue:
                        self.PrintPcdValue(File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValBak, InfMatch, PcdComponentValue, DecMatch, DecDefaultValue, '*M', ModuleGuid)
                    elif DscDefaultValue and DscMatch:
                        if (Pcd.TokenCName, Key, Field) in self.FdfPcdSet:
                            self.PrintPcdValue(File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValBak, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue, '*F')
                        else:
                            self.PrintPcdValue(File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValBak, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue, '*P')


                if ModulePcdSet is None:
                    if IsStructure:
                        continue
                    if not TypeName in ('PATCH', 'FLAG', 'FIXED'):
                        continue
                    if not BuildOptionMatch:
                        ModuleOverride = self.ModulePcdOverride.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName), {})
                        for ModulePath in ModuleOverride:
                            ModuleDefault = ModuleOverride[ModulePath]
                            if Pcd.DatumType in TAB_PCD_NUMERIC_TYPES:
                                if ModuleDefault.startswith('0') and not ModuleDefault.lower().startswith('0x') and \
                                        len(ModuleDefault) > 1 and ModuleDefault.lstrip('0'):
                                    ModuleDefault = ModuleDefault.lstrip('0')
                                ModulePcdDefaultValueNumber = int(ModuleDefault.strip(), 0)
                                Match = (ModulePcdDefaultValueNumber == PcdValueNumber)
                                if Pcd.DatumType == 'BOOLEAN':
                                    ModuleDefault = str(ModulePcdDefaultValueNumber)
                            else:
                                Match = (ModuleDefault.strip() == PcdValue.strip())
                            if Match:
                                continue
                            IsByteArray, ArrayList = ByteArrayForamt(ModuleDefault.strip())
                            if IsByteArray:
                                FileWrite(File, ' *M     %-*s = %s' % (self.MaxLen + 15, ModulePath, '{'))
                                for Array in ArrayList:
                                    FileWrite(File, Array)
                            else:
                                Value =  ModuleDefault.strip()
                                if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                                    if Value.startswith(('0x', '0X')):
                                        Value = '{} ({:d})'.format(Value, int(Value, 0))
                                    else:
                                        Value = "0x{:X} ({})".format(int(Value, 0), Value)
                                FileWrite(File, ' *M     %-*s = %s' % (self.MaxLen + 15, ModulePath, Value))

        if ModulePcdSet is None:
            FileWrite(File, gSectionEnd)
        else:
            if not ReportSubType and ModulePcdSet:
                FileWrite(File, gSubSectionEnd)

    def ParseStruct(self, struct):
        HasDscOverride = False
        if struct:
            for _, Values in list(struct.items()):
                for Key, value in Values.items():
                    if value[1] and value[1].endswith('.dsc'):
                        HasDscOverride = True
                        break
                if HasDscOverride == True:
                    break
        return HasDscOverride

    def PrintPcdDefault(self, File, Pcd, IsStructure, DscMatch, DscDefaultValue, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue):
        if not DscMatch and DscDefaultValue is not None:
            Value = DscDefaultValue.strip()
            IsByteArray, ArrayList = ByteArrayForamt(Value)
            if IsByteArray:
                FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'DSC DEFAULT', "{"))
                for Array in ArrayList:
                    FileWrite(File, Array)
            else:
                if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                    if Value.startswith(('0x', '0X')):
                        Value = '{} ({:d})'.format(Value, int(Value, 0))
                    else:
                        Value = "0x{:X} ({})".format(int(Value, 0), Value)
                FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'DSC DEFAULT', Value))
        if not InfMatch and InfDefaultValue is not None:
            Value = InfDefaultValue.strip()
            IsByteArray, ArrayList = ByteArrayForamt(Value)
            if IsByteArray:
                FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'INF DEFAULT', "{"))
                for Array in ArrayList:
                    FileWrite(File, Array)
            else:
                if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                    if Value.startswith(('0x', '0X')):
                        Value = '{} ({:d})'.format(Value, int(Value, 0))
                    else:
                        Value = "0x{:X} ({})".format(int(Value, 0), Value)
                FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'INF DEFAULT', Value))

        if not DecMatch and DecDefaultValue is not None:
            Value = DecDefaultValue.strip()
            IsByteArray, ArrayList = ByteArrayForamt(Value)
            if IsByteArray:
                FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'DEC DEFAULT', "{"))
                for Array in ArrayList:
                    FileWrite(File, Array)
            else:
                if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                    if Value.startswith(('0x', '0X')):
                        Value = '{} ({:d})'.format(Value, int(Value, 0))
                    else:
                        Value = "0x{:X} ({})".format(int(Value, 0), Value)
                FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'DEC DEFAULT', Value))
            if IsStructure:
                for filedvalues in Pcd.DefaultValues.values():
                    self.PrintStructureInfo(File, filedvalues)
        if DecMatch and IsStructure:
            for filedvalues in Pcd.DefaultValues.values():
                self.PrintStructureInfo(File, filedvalues)

    def PrintPcdValue(self, File, Pcd, PcdTokenCName, TypeName, IsStructure, DscMatch, DscDefaultValue, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue, Flag = '  ',ModuleGuid=None):
        if not Pcd.SkuInfoList:
            Value = Pcd.DefaultValue
            IsByteArray, ArrayList = ByteArrayForamt(Value)
            if IsByteArray:
                FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '{'))
                for Array in ArrayList:
                    FileWrite(File, Array)
            else:
                if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                    if Value.startswith('0') and not Value.lower().startswith('0x') and len(Value) > 1 and Value.lstrip('0'):
                        Value = Value.lstrip('0')
                    if Value.startswith(('0x', '0X')):
                        Value = '{} ({:d})'.format(Value, int(Value, 0))
                    else:
                        Value = "0x{:X} ({})".format(int(Value, 0), Value)
                FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', Value))
            if IsStructure:
                FiledOverrideFlag = False
                if (Pcd.TokenCName,Pcd.TokenSpaceGuidCName) in GlobalData.gPcdSkuOverrides:
                    OverrideValues = GlobalData.gPcdSkuOverrides[(Pcd.TokenCName,Pcd.TokenSpaceGuidCName)]
                else:
                    OverrideValues = Pcd.SkuOverrideValues
                FieldOverrideValues = None
                if OverrideValues:
                    for Data in OverrideValues.values():
                        Struct = list(Data.values())
                        if Struct:
                            FieldOverrideValues = Struct[0]
                            FiledOverrideFlag = True
                            break
                if Pcd.PcdFiledValueFromDscComponent and ModuleGuid and ModuleGuid.replace("-","S") in Pcd.PcdFiledValueFromDscComponent:
                    FieldOverrideValues = Pcd.PcdFiledValueFromDscComponent[ModuleGuid.replace("-","S")]
                if FieldOverrideValues:
                    OverrideFieldStruct = self.OverrideFieldValue(Pcd, FieldOverrideValues)
                    self.PrintStructureInfo(File, OverrideFieldStruct)

                if not FiledOverrideFlag and (Pcd.PcdFieldValueFromComm or Pcd.PcdFieldValueFromFdf):
                    OverrideFieldStruct = self.OverrideFieldValue(Pcd, {})
                    self.PrintStructureInfo(File, OverrideFieldStruct)
            self.PrintPcdDefault(File, Pcd, IsStructure, DscMatch, DscDefaultValue, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue)
        else:
            FirstPrint = True
            SkuList = sorted(Pcd.SkuInfoList.keys())
            for Sku in SkuList:
                SkuInfo = Pcd.SkuInfoList[Sku]
                SkuIdName = SkuInfo.SkuIdName
                if TypeName in ('DYNHII', 'DEXHII'):
                    if SkuInfo.DefaultStoreDict:
                        DefaultStoreList = sorted(SkuInfo.DefaultStoreDict.keys())
                        for DefaultStore in DefaultStoreList:
                            Value = SkuInfo.DefaultStoreDict[DefaultStore]
                            IsByteArray, ArrayList = ByteArrayForamt(Value)
                            if Pcd.DatumType == 'BOOLEAN':
                                Value = str(int(Value, 0))
                            if FirstPrint:
                                FirstPrint = False
                                if IsByteArray:
                                    if self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '{'))
                                    elif self.DefaultStoreSingle and not self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', '{'))
                                    elif not self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + DefaultStore + ')', '{'))
                                    else:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', '(' + DefaultStore + ')', '{'))
                                    for Array in ArrayList:
                                        FileWrite(File, Array)
                                else:
                                    if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                                        if Value.startswith(('0x', '0X')):
                                            Value = '{} ({:d})'.format(Value, int(Value, 0))
                                        else:
                                            Value = "0x{:X} ({})".format(int(Value, 0), Value)
                                    if self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', Value))
                                    elif self.DefaultStoreSingle and not self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', Value))
                                    elif not self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + DefaultStore + ')', Value))
                                    else:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', '(' + DefaultStore + ')', Value))
                            else:
                                if IsByteArray:
                                    if self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '{'))
                                    elif self.DefaultStoreSingle and not self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', '{'))
                                    elif not self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + DefaultStore + ')', '{'))
                                    else:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', '(' + DefaultStore + ')', '{'))
                                    for Array in ArrayList:
                                        FileWrite(File, Array)
                                else:
                                    if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                                        if Value.startswith(('0x', '0X')):
                                            Value = '{} ({:d})'.format(Value, int(Value, 0))
                                        else:
                                            Value = "0x{:X} ({})".format(int(Value, 0), Value)
                                    if self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')',  Value))
                                    elif self.DefaultStoreSingle and not self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', Value))
                                    elif not self.DefaultStoreSingle and self.SkuSingle:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + DefaultStore + ')', Value))
                                    else:
                                        FileWrite(File, ' %-*s   : %6s %10s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', '(' + DefaultStore + ')', Value))
                            FileWrite(File, '%*s: %s: %s' % (self.MaxLen + 4, SkuInfo.VariableGuid, SkuInfo.VariableName, SkuInfo.VariableOffset))
                            if IsStructure:
                                OverrideValues = Pcd.SkuOverrideValues.get(Sku)
                                if OverrideValues:
                                    OverrideFieldStruct = self.OverrideFieldValue(Pcd, OverrideValues[DefaultStore])
                                    self.PrintStructureInfo(File, OverrideFieldStruct)
                            self.PrintPcdDefault(File, Pcd, IsStructure, DscMatch, DscDefaultValue, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue)
                else:
                    Value = SkuInfo.DefaultValue
                    IsByteArray, ArrayList = ByteArrayForamt(Value)
                    if Pcd.DatumType == 'BOOLEAN':
                        Value = str(int(Value, 0))
                    if FirstPrint:
                        FirstPrint = False
                        if IsByteArray:
                            if self.SkuSingle:
                                FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', "{"))
                            else:
                                FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', "{"))
                            for Array in ArrayList:
                                FileWrite(File, Array)
                        else:
                            if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                                if Value.startswith(('0x', '0X')):
                                    Value = '{} ({:d})'.format(Value, int(Value, 0))
                                else:
                                    Value = "0x{:X} ({})".format(int(Value, 0), Value)
                            if self.SkuSingle:
                                FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', Value))
                            else:
                                FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, Flag + ' ' + PcdTokenCName, TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', Value))
                    else:
                        if IsByteArray:
                            if self.SkuSingle:
                                FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', "{"))
                            else:
                                FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', "{"))
                            for Array in ArrayList:
                                FileWrite(File, Array)
                        else:
                            if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
                                if Value.startswith(('0x', '0X')):
                                    Value = '{} ({:d})'.format(Value, int(Value, 0))
                                else:
                                    Value = "0x{:X} ({})".format(int(Value, 0), Value)
                            if self.SkuSingle:
                                FileWrite(File, ' %-*s   : %6s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', Value))
                            else:
                                FileWrite(File, ' %-*s   : %6s %10s %10s = %s' % (self.MaxLen, ' ', TypeName, '(' + Pcd.DatumType + ')', '(' + SkuIdName + ')', Value))
                    if TypeName in ('DYNVPD', 'DEXVPD'):
                        FileWrite(File, '%*s' % (self.MaxLen + 4, SkuInfo.VpdOffset))
                        VPDPcdItem = (Pcd.TokenSpaceGuidCName + '.' + PcdTokenCName, SkuIdName, SkuInfo.VpdOffset, Pcd.MaxDatumSize, SkuInfo.DefaultValue)
                        if VPDPcdItem not in VPDPcdList:
                            PcdGuidList = self.UnusedPcds.get(Pcd.TokenSpaceGuidCName)
                            if PcdGuidList:
                                PcdList = PcdGuidList.get(Pcd.Type)
                                if not PcdList:
                                    VPDPcdList.append(VPDPcdItem)
                                for VpdPcd in PcdList:
                                    if PcdTokenCName == VpdPcd.TokenCName:
                                        break
                                else:
                                    VPDPcdList.append(VPDPcdItem)
                    if IsStructure:
                        FiledOverrideFlag = False
                        OverrideValues = Pcd.SkuOverrideValues.get(Sku)
                        if OverrideValues:
                            Keys = list(OverrideValues.keys())
                            OverrideFieldStruct = self.OverrideFieldValue(Pcd, OverrideValues[Keys[0]])
                            self.PrintStructureInfo(File, OverrideFieldStruct)
                            FiledOverrideFlag = True
                        if not FiledOverrideFlag and (Pcd.PcdFieldValueFromComm or Pcd.PcdFieldValueFromFdf):
                            OverrideFieldStruct = self.OverrideFieldValue(Pcd, {})
                            self.PrintStructureInfo(File, OverrideFieldStruct)
                    self.PrintPcdDefault(File, Pcd, IsStructure, DscMatch, DscDefaultValue, InfMatch, InfDefaultValue, DecMatch, DecDefaultValue)

    def OverrideFieldValue(self, Pcd, OverrideStruct):
        OverrideFieldStruct = collections.OrderedDict()
        if OverrideStruct:
            for _, Values in OverrideStruct.items():
                for Key,value in Values.items():
                    if value[1] and value[1].endswith('.dsc'):
                        OverrideFieldStruct[Key] = value
        if Pcd.PcdFieldValueFromFdf:
            for Key, Values in Pcd.PcdFieldValueFromFdf.items():
                if Key in OverrideFieldStruct and Values[0] == OverrideFieldStruct[Key][0]:
                    continue
                OverrideFieldStruct[Key] = Values
        if Pcd.PcdFieldValueFromComm:
            for Key, Values in Pcd.PcdFieldValueFromComm.items():
                if Key in OverrideFieldStruct and Values[0] == OverrideFieldStruct[Key][0]:
                    continue
                OverrideFieldStruct[Key] = Values
        return OverrideFieldStruct

    def PrintStructureInfo(self, File, Struct):
        for Key, Value in sorted(Struct.items(), key=lambda x: x[0]):
            if Value[1] and 'build command options' in Value[1]:
                FileWrite(File, '    *B  %-*s = %s' % (self.MaxLen + 4, '.' + Key, Value[0]))
            elif Value[1] and Value[1].endswith('.fdf'):
                FileWrite(File, '    *F  %-*s = %s' % (self.MaxLen + 4, '.' + Key, Value[0]))
            else:
                FileWrite(File, '        %-*s = %s' % (self.MaxLen + 4, '.' + Key, Value[0]))

    def StrtoHex(self, value):
        try:
            value = hex(int(value))
            return value
        except:
            if value.startswith("L\"") and value.endswith("\""):
                valuelist = []
                for ch in value[2:-1]:
                    valuelist.append(hex(ord(ch)))
                    valuelist.append('0x00')
                return valuelist
            elif value.startswith("\"") and value.endswith("\""):
                return hex(ord(value[1:-1]))
            elif value.startswith("{") and value.endswith("}"):
                valuelist = []
                if ',' not in value:
                    return value[1:-1]
                for ch in value[1:-1].split(','):
                    ch = ch.strip()
                    if ch.startswith('0x') or ch.startswith('0X'):
                        valuelist.append(ch)
                        continue
                    try:
                        valuelist.append(hex(int(ch.strip())))
                    except:
                        pass
                return valuelist
            else:
                return value

    def IsStructurePcd(self, PcdToken, PcdTokenSpaceGuid):
        if GlobalData.gStructurePcd and (self.Arch in GlobalData.gStructurePcd) and ((PcdToken, PcdTokenSpaceGuid) in GlobalData.gStructurePcd[self.Arch]):
            return True
        else:
            return False

##
# Reports platform and module Prediction information
#
# This class reports the platform execution order prediction section and
# module load fixed address prediction subsection in the build report file.
#
class PredictionReport(object):
    ##
    # Constructor function for class PredictionReport
    #
    # This constructor function generates PredictionReport object for the platform.
    #
    # @param self:           The object pointer
    # @param Wa              Workspace context information
    #
    def __init__(self, Wa):
        self._MapFileName = os.path.join(Wa.BuildDir, Wa.Name + ".map")
        self._MapFileParsed = False
        self._EotToolInvoked = False
        self._FvDir = Wa.FvDir
        self._EotDir = Wa.BuildDir
        self._FfsEntryPoint = {}
        self._GuidMap = {}
        self._SourceList = []
        self.FixedMapDict = {}
        self.ItemList = []
        self.MaxLen = 0

        #
        # Collect all platform reference source files and GUID C Name
        #
        for Pa in Wa.AutoGenObjectList:
            for Module in Pa.LibraryAutoGenList + Pa.ModuleAutoGenList:
                #
                # BASE typed modules are EFI agnostic, so we need not scan
                # their source code to find PPI/Protocol produce or consume
                # information.
                #
                if Module.ModuleType == SUP_MODULE_BASE:
                    continue
                #
                # Add module referenced source files
                #
                self._SourceList.append(str(Module))
                IncludeList = {}
                for Source in Module.SourceFileList:
                    if os.path.splitext(str(Source))[1].lower() == ".c":
                        self._SourceList.append("  " + str(Source))
                        FindIncludeFiles(Source.Path, Module.IncludePathList, IncludeList)
                for IncludeFile in IncludeList.values():
                    self._SourceList.append("  " + IncludeFile)

                for Guid in Module.PpiList:
                    self._GuidMap[Guid] = GuidStructureStringToGuidString(Module.PpiList[Guid])
                for Guid in Module.ProtocolList:
                    self._GuidMap[Guid] = GuidStructureStringToGuidString(Module.ProtocolList[Guid])
                for Guid in Module.GuidList:
                    self._GuidMap[Guid] = GuidStructureStringToGuidString(Module.GuidList[Guid])

                if Module.Guid and not Module.IsLibrary:
                    EntryPoint = " ".join(Module.Module.ModuleEntryPointList)

                    RealEntryPoint = "_ModuleEntryPoint"

                    self._FfsEntryPoint[Module.Guid.upper()] = (EntryPoint, RealEntryPoint)


        #
        # Collect platform firmware volume list as the input of EOT.
        #
        self._FvList = []
        if Wa.FdfProfile:
            for Fd in Wa.FdfProfile.FdDict:
                for FdRegion in Wa.FdfProfile.FdDict[Fd].RegionList:
                    if FdRegion.RegionType != BINARY_FILE_TYPE_FV:
                        continue
                    for FvName in FdRegion.RegionDataList:
                        if FvName in self._FvList:
                            continue
                        self._FvList.append(FvName)
                        for Ffs in Wa.FdfProfile.FvDict[FvName.upper()].FfsList:
                            for Section in Ffs.SectionList:
                                try:
                                    for FvSection in Section.SectionList:
                                        if FvSection.FvName in self._FvList:
                                            continue
                                        self._FvList.append(FvSection.FvName)
                                except AttributeError:
                                    pass


    ##
    # Parse platform fixed address map files
    #
    # This function parses the platform final fixed address map file to get
    # the database of predicted fixed address for module image base, entry point
    # etc.
    #
    # @param self:           The object pointer
    #
    def _ParseMapFile(self):
        if self._MapFileParsed:
            return
        self._MapFileParsed = True
        if os.path.isfile(self._MapFileName):
            try:
                FileContents = open(self._MapFileName).read()
                for Match in gMapFileItemPattern.finditer(FileContents):
                    AddressType = Match.group(1)
                    BaseAddress = Match.group(2)
                    EntryPoint = Match.group(3)
                    Guid = Match.group(4).upper()
                    List = self.FixedMapDict.setdefault(Guid, [])
                    List.append((AddressType, BaseAddress, "*I"))
                    List.append((AddressType, EntryPoint, "*E"))
            except:
                EdkLogger.warn(None, "Cannot open file to read", self._MapFileName)

    ##
    # Invokes EOT tool to get the predicted the execution order.
    #
    # This function invokes EOT tool to calculate the predicted dispatch order
    #
    # @param self:           The object pointer
    #
    def _InvokeEotTool(self):
        if self._EotToolInvoked:
            return

        self._EotToolInvoked = True
        FvFileList = []
        for FvName in self._FvList:
            FvFile = os.path.join(self._FvDir, FvName + ".Fv")
            if os.path.isfile(FvFile):
                FvFileList.append(FvFile)

        if len(FvFileList) == 0:
            return
        #
        # Write source file list and GUID file list to an intermediate file
        # as the input for EOT tool and dispatch List as the output file
        # from EOT tool.
        #
        SourceList = os.path.join(self._EotDir, "SourceFile.txt")
        GuidList = os.path.join(self._EotDir, "GuidList.txt")
        DispatchList = os.path.join(self._EotDir, "Dispatch.txt")

        TempFile = []
        for Item in self._SourceList:
            FileWrite(TempFile, Item)
        SaveFileOnChange(SourceList, "".join(TempFile), False)
        TempFile = []
        for Key in self._GuidMap:
            FileWrite(TempFile, "%s %s" % (Key, self._GuidMap[Key]))
        SaveFileOnChange(GuidList, "".join(TempFile), False)

        try:
            from Eot.EotMain import Eot

            #
            # Invoke EOT tool and echo its runtime performance
            #
            EotStartTime = time.time()
            Eot(CommandLineOption=False, SourceFileList=SourceList, GuidList=GuidList,
                FvFileList=' '.join(FvFileList), Dispatch=DispatchList, IsInit=True)
            EotEndTime = time.time()
            EotDuration = time.strftime("%H:%M:%S", time.gmtime(int(round(EotEndTime - EotStartTime))))
            EdkLogger.quiet("EOT run time: %s\n" % EotDuration)

            #
            # Parse the output of EOT tool
            #
            for Line in open(DispatchList):
                if len(Line.split()) < 4:
                    continue
                (Guid, Phase, FfsName, FilePath) = Line.split()
                Symbol = self._FfsEntryPoint.get(Guid, [FfsName, ""])[0]
                if len(Symbol) > self.MaxLen:
                    self.MaxLen = len(Symbol)
                self.ItemList.append((Phase, Symbol, FilePath))
        except:
            EdkLogger.quiet("(Python %s on %s\n%s)" % (platform.python_version(), sys.platform, traceback.format_exc()))
            EdkLogger.warn(None, "Failed to generate execution order prediction report, for some error occurred in executing EOT.")


    ##
    # Generate platform execution order report
    #
    # This function generates the predicted module execution order.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    #
    def _GenerateExecutionOrderReport(self, File):
        self._InvokeEotTool()
        if len(self.ItemList) == 0:
            return
        FileWrite(File, gSectionStart)
        FileWrite(File, "Execution Order Prediction")
        FileWrite(File, "*P PEI phase")
        FileWrite(File, "*D DXE phase")
        FileWrite(File, "*E Module INF entry point name")
        FileWrite(File, "*N Module notification function name")

        FileWrite(File, "Type %-*s %s" % (self.MaxLen, "Symbol", "Module INF Path"))
        FileWrite(File, gSectionSep)
        for Item in self.ItemList:
            FileWrite(File, "*%sE  %-*s %s" % (Item[0], self.MaxLen, Item[1], Item[2]))

        FileWrite(File, gSectionStart)

    ##
    # Generate Fixed Address report.
    #
    # This function generate the predicted fixed address report for a module
    # specified by Guid.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    # @param Guid            The module Guid value.
    # @param NotifyList      The list of all notify function in a module
    #
    def _GenerateFixedAddressReport(self, File, Guid, NotifyList):
        self._ParseMapFile()
        FixedAddressList = self.FixedMapDict.get(Guid)
        if not FixedAddressList:
            return

        FileWrite(File, gSubSectionStart)
        FileWrite(File, "Fixed Address Prediction")
        FileWrite(File, "*I  Image Loading Address")
        FileWrite(File, "*E  Entry Point Address")
        FileWrite(File, "*N  Notification Function Address")
        FileWrite(File, "*F  Flash Address")
        FileWrite(File, "*M  Memory Address")
        FileWrite(File, "*S  SMM RAM Offset")
        FileWrite(File, "TOM Top of Memory")

        FileWrite(File, "Type Address           Name")
        FileWrite(File, gSubSectionSep)
        for Item in FixedAddressList:
            Type = Item[0]
            Value = Item[1]
            Symbol = Item[2]
            if Symbol == "*I":
                Name = "(Image Base)"
            elif Symbol == "*E":
                Name = self._FfsEntryPoint.get(Guid, ["", "_ModuleEntryPoint"])[1]
            elif Symbol in NotifyList:
                Name = Symbol
                Symbol = "*N"
            else:
                continue

            if "Flash" in Type:
                Symbol += "F"
            elif "Memory" in Type:
                Symbol += "M"
            else:
                Symbol += "S"

            if Value[0] == "-":
                Value = "TOM" + Value

            FileWrite(File, "%s  %-16s  %s" % (Symbol, Value, Name))

    ##
    # Generate report for the prediction part
    #
    # This function generate the predicted fixed address report for a module or
    # predicted module execution order for a platform.
    # If the input Guid is None, then, it generates the predicted module execution order;
    # otherwise it generated the module fixed loading address for the module specified by
    # Guid.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    # @param Guid            The module Guid value.
    #
    def GenerateReport(self, File, Guid):
        if Guid:
            self._GenerateFixedAddressReport(File, Guid.upper(), [])
        else:
            self._GenerateExecutionOrderReport(File)

##
# Reports FD region information
#
# This class reports the FD subsection in the build report file.
# It collects region information of platform flash device.
# If the region is a firmware volume, it lists the set of modules
# and its space information; otherwise, it only lists its region name,
# base address and size in its sub-section header.
# If there are nesting FVs, the nested FVs will list immediate after
# this FD region subsection
#
class FdRegionReport(object):
    ##
    # Discover all the nested FV name list.
    #
    # This is an internal worker function to discover the all the nested FV information
    # in the parent firmware volume. It uses deep first search algorithm recursively to
    # find all the FV list name and append them to the list.
    #
    # @param self            The object pointer
    # @param FvName          The name of current firmware file system
    # @param Wa              Workspace context information
    #
    def _DiscoverNestedFvList(self, FvName, Wa):
        FvDictKey=FvName.upper()
        if FvDictKey in Wa.FdfProfile.FvDict:
            for Ffs in Wa.FdfProfile.FvDict[FvName.upper()].FfsList:
                for Section in Ffs.SectionList:
                    try:
                        for FvSection in Section.SectionList:
                            if FvSection.FvName in self.FvList:
                                continue
                            self._GuidsDb[Ffs.NameGuid.upper()] = FvSection.FvName
                            self.FvList.append(FvSection.FvName)
                            self.FvInfo[FvSection.FvName] = ("Nested FV", 0, 0)
                            self._DiscoverNestedFvList(FvSection.FvName, Wa)
                    except AttributeError:
                        pass

    ##
    # Constructor function for class FdRegionReport
    #
    # This constructor function generates FdRegionReport object for a specified FdRegion.
    # If the FdRegion is a firmware volume, it will recursively find all its nested Firmware
    # volume list. This function also collects GUID map in order to dump module identification
    # in the final report.
    #
    # @param self:           The object pointer
    # @param FdRegion        The current FdRegion object
    # @param Wa              Workspace context information
    #
    def __init__(self, FdRegion, Wa):
        self.Type = FdRegion.RegionType
        self.BaseAddress = FdRegion.Offset
        self.Size = FdRegion.Size
        self.FvList = []
        self.FvInfo = {}
        self._GuidsDb = {}
        self._FvDir = Wa.FvDir
        self._WorkspaceDir = Wa.WorkspaceDir

        #
        # If the input FdRegion is not a firmware volume,
        # we are done.
        #
        if self.Type != BINARY_FILE_TYPE_FV:
            return

        #
        # Find all nested FVs in the FdRegion
        #
        for FvName in FdRegion.RegionDataList:
            if FvName in self.FvList:
                continue
            self.FvList.append(FvName)
            self.FvInfo[FvName] = ("Fd Region", self.BaseAddress, self.Size)
            self._DiscoverNestedFvList(FvName, Wa)

        PlatformPcds = {}
        #
        # Collect PCDs declared in DEC files.
        #
        for Pa in Wa.AutoGenObjectList:
            for Package in Pa.PackageList:
                for (TokenCName, TokenSpaceGuidCName, DecType) in Package.Pcds:
                    DecDefaultValue = Package.Pcds[TokenCName, TokenSpaceGuidCName, DecType].DefaultValue
                    PlatformPcds[(TokenCName, TokenSpaceGuidCName)] = DecDefaultValue
        #
        # Collect PCDs defined in DSC file
        #
        for Pa in Wa.AutoGenObjectList:
            for (TokenCName, TokenSpaceGuidCName) in Pa.Platform.Pcds:
                DscDefaultValue = Pa.Platform.Pcds[(TokenCName, TokenSpaceGuidCName)].DefaultValue
                PlatformPcds[(TokenCName, TokenSpaceGuidCName)] = DscDefaultValue

        #
        # Add PEI and DXE a priori files GUIDs defined in PI specification.
        #
        self._GuidsDb[PEI_APRIORI_GUID] = "PEI Apriori"
        self._GuidsDb[DXE_APRIORI_GUID] = "DXE Apriori"
        #
        # Add ACPI table storage file
        #
        self._GuidsDb["7E374E25-8E01-4FEE-87F2-390C23C606CD"] = "ACPI table storage"

        for Pa in Wa.AutoGenObjectList:
            for ModuleKey in Pa.Platform.Modules:
                M = Pa.Platform.Modules[ModuleKey].M
                InfPath = mws.join(Wa.WorkspaceDir, M.MetaFile.File)
                self._GuidsDb[M.Guid.upper()] = "%s (%s)" % (M.Module.BaseName, InfPath)

        #
        # Collect the GUID map in the FV firmware volume
        #
        for FvName in self.FvList:
            FvDictKey=FvName.upper()
            if FvDictKey in Wa.FdfProfile.FvDict:
                for Ffs in Wa.FdfProfile.FvDict[FvName.upper()].FfsList:
                    try:
                        #
                        # collect GUID map for binary EFI file in FDF file.
                        #
                        Guid = Ffs.NameGuid.upper()
                        Match = gPcdGuidPattern.match(Ffs.NameGuid)
                        if Match:
                            PcdTokenspace = Match.group(1)
                            PcdToken = Match.group(2)
                            if (PcdToken, PcdTokenspace) in PlatformPcds:
                                GuidValue = PlatformPcds[(PcdToken, PcdTokenspace)]
                                Guid = GuidStructureByteArrayToGuidString(GuidValue).upper()
                        for Section in Ffs.SectionList:
                            try:
                                ModuleSectFile = mws.join(Wa.WorkspaceDir, Section.SectFileName)
                                self._GuidsDb[Guid] = ModuleSectFile
                            except AttributeError:
                                pass
                    except AttributeError:
                        pass


    ##
    # Internal worker function to generate report for the FD region
    #
    # This internal worker function to generate report for the FD region.
    # It the type is firmware volume, it lists offset and module identification.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    # @param Title           The title for the FD subsection
    # @param BaseAddress     The base address for the FD region
    # @param Size            The size of the FD region
    # @param FvName          The FV name if the FD region is a firmware volume
    #
    def _GenerateReport(self, File, Title, Type, BaseAddress, Size=0, FvName=None):
        FileWrite(File, gSubSectionStart)
        FileWrite(File, Title)
        FileWrite(File, "Type:               %s" % Type)
        FileWrite(File, "Base Address:       0x%X" % BaseAddress)

        if self.Type == BINARY_FILE_TYPE_FV:
            FvTotalSize = 0
            FvTakenSize = 0
            FvFreeSize  = 0
            if FvName.upper().endswith('.FV'):
                FileExt = FvName + ".txt"
            else:
                FileExt = FvName + ".Fv.txt"

            if not os.path.isfile(FileExt):
                FvReportFileName = mws.join(self._WorkspaceDir, FileExt)
                if not os.path.isfile(FvReportFileName):
                    FvReportFileName = os.path.join(self._FvDir, FileExt)
            try:
                #
                # Collect size info in the firmware volume.
                #
                FvReport = open(FvReportFileName).read()
                Match = gFvTotalSizePattern.search(FvReport)
                if Match:
                    FvTotalSize = int(Match.group(1), 16)
                Match = gFvTakenSizePattern.search(FvReport)
                if Match:
                    FvTakenSize = int(Match.group(1), 16)
                FvFreeSize = FvTotalSize - FvTakenSize
                #
                # Write size information to the report file.
                #
                FileWrite(File, "Size:               0x%X (%.0fK)" % (FvTotalSize, FvTotalSize / 1024.0))
                FileWrite(File, "Fv Name:            %s (%.1f%% Full)" % (FvName, FvTakenSize * 100.0 / FvTotalSize))
                FileWrite(File, "Occupied Size:      0x%X (%.0fK)" % (FvTakenSize, FvTakenSize / 1024.0))
                FileWrite(File, "Free Size:          0x%X (%.0fK)" % (FvFreeSize, FvFreeSize / 1024.0))
                FileWrite(File, "Offset     Module")
                FileWrite(File, gSubSectionSep)
                #
                # Write module offset and module identification to the report file.
                #
                OffsetInfo = {}
                for Match in gOffsetGuidPattern.finditer(FvReport):
                    Guid = Match.group(2).upper()
                    OffsetInfo[Match.group(1)] = self._GuidsDb.get(Guid, Guid)
                OffsetList = sorted(OffsetInfo.keys())
                for Offset in OffsetList:
                    FileWrite (File, "%s %s" % (Offset, OffsetInfo[Offset]))
            except IOError:
                EdkLogger.warn(None, "Fail to read report file", FvReportFileName)
        else:
            FileWrite(File, "Size:               0x%X (%.0fK)" % (Size, Size / 1024.0))
        FileWrite(File, gSubSectionEnd)

    ##
    # Generate report for the FD region
    #
    # This function generates report for the FD region.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    #
    def GenerateReport(self, File):
        if (len(self.FvList) > 0):
            for FvItem in self.FvList:
                Info = self.FvInfo[FvItem]
                self._GenerateReport(File, Info[0], TAB_FV_DIRECTORY, Info[1], Info[2], FvItem)
        else:
            self._GenerateReport(File, "FD Region", self.Type, self.BaseAddress, self.Size)

##
# Reports FD information
#
# This class reports the FD section in the build report file.
# It collects flash device information for a platform.
#
class FdReport(object):
    ##
    # Constructor function for class FdReport
    #
    # This constructor function generates FdReport object for a specified
    # firmware device.
    #
    # @param self            The object pointer
    # @param Fd              The current Firmware device object
    # @param Wa              Workspace context information
    #
    def __init__(self, Fd, Wa):
        self.FdName = Fd.FdUiName
        self.BaseAddress = Fd.BaseAddress
        self.Size = Fd.Size
        self.FdRegionList = [FdRegionReport(FdRegion, Wa) for FdRegion in Fd.RegionList]
        self.FvPath = os.path.join(Wa.BuildDir, TAB_FV_DIRECTORY)
        self.VPDBaseAddress = 0
        self.VPDSize = 0
        for index, FdRegion in enumerate(Fd.RegionList):
            if str(FdRegion.RegionType) == 'FILE' and Wa.Platform.VpdToolGuid in str(FdRegion.RegionDataList):
                self.VPDBaseAddress = self.FdRegionList[index].BaseAddress
                self.VPDSize = self.FdRegionList[index].Size
                break

    ##
    # Generate report for the firmware device.
    #
    # This function generates report for the firmware device.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    #
    def GenerateReport(self, File):
        FileWrite(File, gSectionStart)
        FileWrite(File, "Firmware Device (FD)")
        FileWrite(File, "FD Name:            %s" % self.FdName)
        FileWrite(File, "Base Address:       %s" % self.BaseAddress)
        FileWrite(File, "Size:               0x%X (%.0fK)" % (self.Size, self.Size / 1024.0))
        if len(self.FdRegionList) > 0:
            FileWrite(File, gSectionSep)
            for FdRegionItem in self.FdRegionList:
                FdRegionItem.GenerateReport(File)

        if VPDPcdList:
            VPDPcdList.sort(key=lambda x: int(x[2], 0))
            FileWrite(File, gSubSectionStart)
            FileWrite(File, "FD VPD Region")
            FileWrite(File, "Base Address:       0x%X" % self.VPDBaseAddress)
            FileWrite(File, "Size:               0x%X (%.0fK)" % (self.VPDSize, self.VPDSize / 1024.0))
            FileWrite(File, gSubSectionSep)
            for item in VPDPcdList:
                # Add BaseAddress for offset
                Offset = '0x%08X' % (int(item[2], 16) + self.VPDBaseAddress)
                IsByteArray, ArrayList = ByteArrayForamt(item[-1])
                Skuinfo = item[1]
                if len(GlobalData.gSkuids) == 1 :
                    Skuinfo = GlobalData.gSkuids[0]
                if IsByteArray:
                    FileWrite(File, "%s | %s | %s | %s | %s" % (item[0], Skuinfo, Offset, item[3], '{'))
                    for Array in ArrayList:
                        FileWrite(File, Array)
                else:
                    FileWrite(File, "%s | %s | %s | %s | %s" % (item[0], Skuinfo, Offset, item[3], item[-1]))
            FileWrite(File, gSubSectionEnd)
        FileWrite(File, gSectionEnd)



##
# Reports platform information
#
# This class reports the whole platform information
#
class PlatformReport(object):
    ##
    # Constructor function for class PlatformReport
    #
    # This constructor function generates PlatformReport object a platform build.
    # It generates report for platform summary, flash, global PCDs and detailed
    # module information for modules involved in platform build.
    #
    # @param self            The object pointer
    # @param Wa              Workspace context information
    # @param MaList          The list of modules in the platform build
    #
    def __init__(self, Wa, MaList, ReportType):
        self._WorkspaceDir = Wa.WorkspaceDir
        self.PlatformName = Wa.Name
        self.PlatformDscPath = Wa.Platform
        self.Architectures = " ".join(Wa.ArchList)
        self.ToolChain = Wa.ToolChain
        self.Target = Wa.BuildTarget
        self.OutputPath = os.path.join(Wa.WorkspaceDir, Wa.OutputDir)
        self.BuildEnvironment = platform.platform()

        self.PcdReport = None
        if "PCD" in ReportType:
            self.PcdReport = PcdReport(Wa)

        self.FdReportList = []
        if "FLASH" in ReportType and Wa.FdfProfile and MaList is None:
            for Fd in Wa.FdfProfile.FdDict:
                self.FdReportList.append(FdReport(Wa.FdfProfile.FdDict[Fd], Wa))

        self.PredictionReport = None
        if "FIXED_ADDRESS" in ReportType or "EXECUTION_ORDER" in ReportType:
            self.PredictionReport = PredictionReport(Wa)

        self.DepexParser = None
        if "DEPEX" in ReportType:
            self.DepexParser = DepexParser(Wa)

        self.ModuleReportList = []
        if MaList is not None:
            self._IsModuleBuild = True
            for Ma in MaList:
                self.ModuleReportList.append(ModuleReport(Ma, ReportType))
        else:
            self._IsModuleBuild = False
            for Pa in Wa.AutoGenObjectList:
                ModuleAutoGenList = []
                for ModuleKey in Pa.Platform.Modules:
                    ModuleAutoGenList.append(Pa.Platform.Modules[ModuleKey].M)
                if GlobalData.gFdfParser is not None:
                    if Pa.Arch in GlobalData.gFdfParser.Profile.InfDict:
                        INFList = GlobalData.gFdfParser.Profile.InfDict[Pa.Arch]
                        for InfName in INFList:
                            InfClass = PathClass(NormPath(InfName), Wa.WorkspaceDir, Pa.Arch)
                            Ma = ModuleAutoGen(Wa, InfClass, Pa.BuildTarget, Pa.ToolChain, Pa.Arch, Wa.MetaFile, Pa.DataPipe)
                            if Ma is None:
                                continue
                            if Ma not in ModuleAutoGenList:
                                ModuleAutoGenList.append(Ma)
                for MGen in ModuleAutoGenList:
                    self.ModuleReportList.append(ModuleReport(MGen, ReportType))



    ##
    # Generate report for the whole platform.
    #
    # This function generates report for platform information.
    # It comprises of platform summary, global PCD, flash and
    # module list sections.
    #
    # @param self            The object pointer
    # @param File            The file object for report
    # @param BuildDuration   The total time to build the modules
    # @param AutoGenTime     The total time of AutoGen Phase
    # @param MakeTime        The total time of Make Phase
    # @param GenFdsTime      The total time of GenFds Phase
    # @param ReportType      The kind of report items in the final report file
    #
    def GenerateReport(self, File, BuildDuration, AutoGenTime, MakeTime, GenFdsTime, ReportType):
        FileWrite(File, "Platform Summary")
        FileWrite(File, "Platform Name:        %s" % self.PlatformName)
        FileWrite(File, "Platform DSC Path:    %s" % self.PlatformDscPath)
        FileWrite(File, "Architectures:        %s" % self.Architectures)
        FileWrite(File, "Tool Chain:           %s" % self.ToolChain)
        FileWrite(File, "Target:               %s" % self.Target)
        if GlobalData.gSkuids:
            FileWrite(File, "SKUID:                %s" % " ".join(GlobalData.gSkuids))
        if GlobalData.gDefaultStores:
            FileWrite(File, "DefaultStore:         %s" % " ".join(GlobalData.gDefaultStores))
        FileWrite(File, "Output Path:          %s" % self.OutputPath)
        FileWrite(File, "Build Environment:    %s" % self.BuildEnvironment)
        FileWrite(File, "Build Duration:       %s" % BuildDuration)
        if AutoGenTime:
            FileWrite(File, "AutoGen Duration:     %s" % AutoGenTime)
        if MakeTime:
            FileWrite(File, "Make Duration:        %s" % MakeTime)
        if GenFdsTime:
            FileWrite(File, "GenFds Duration:      %s" % GenFdsTime)
        FileWrite(File, "Report Content:       %s" % ", ".join(ReportType))

        if GlobalData.MixedPcd:
            FileWrite(File, gSectionStart)
            FileWrite(File, "The following PCDs use different access methods:")
            FileWrite(File, gSectionSep)
            for PcdItem in GlobalData.MixedPcd:
                FileWrite(File, "%s.%s" % (str(PcdItem[1]), str(PcdItem[0])))
            FileWrite(File, gSectionEnd)

        if not self._IsModuleBuild:
            if "PCD" in ReportType:
                self.PcdReport.GenerateReport(File, None)

            if "FLASH" in ReportType:
                for FdReportListItem in self.FdReportList:
                    FdReportListItem.GenerateReport(File)

        for ModuleReportItem in self.ModuleReportList:
            ModuleReportItem.GenerateReport(File, self.PcdReport, self.PredictionReport, self.DepexParser, ReportType)

        if not self._IsModuleBuild:
            if "EXECUTION_ORDER" in ReportType:
                self.PredictionReport.GenerateReport(File, None)

## BuildReport class
#
#  This base class contain the routines to collect data and then
#  applies certain format to the output report
#
class BuildReport(object):
    ##
    # Constructor function for class BuildReport
    #
    # This constructor function generates BuildReport object a platform build.
    # It generates report for platform summary, flash, global PCDs and detailed
    # module information for modules involved in platform build.
    #
    # @param self            The object pointer
    # @param ReportFile      The file name to save report file
    # @param ReportType      The kind of report items in the final report file
    #
    def __init__(self, ReportFile, ReportType):
        self.ReportFile = ReportFile
        if ReportFile:
            self.ReportList = []
            self.ReportType = []
            if ReportType:
                for ReportTypeItem in ReportType:
                    if ReportTypeItem not in self.ReportType:
                        self.ReportType.append(ReportTypeItem)
            else:
                self.ReportType = ["PCD", "LIBRARY", "BUILD_FLAGS", "DEPEX", "HASH", "FLASH", "FIXED_ADDRESS"]
    ##
    # Adds platform report to the list
    #
    # This function adds a platform report to the final report list.
    #
    # @param self            The object pointer
    # @param Wa              Workspace context information
    # @param MaList          The list of modules in the platform build
    #
    def AddPlatformReport(self, Wa, MaList=None):
        if self.ReportFile:
            self.ReportList.append((Wa, MaList))

    ##
    # Generates the final report.
    #
    # This function generates platform build report. It invokes GenerateReport()
    # method for every platform report in the list.
    #
    # @param self            The object pointer
    # @param BuildDuration   The total time to build the modules
    # @param AutoGenTime     The total time of AutoGen phase
    # @param MakeTime        The total time of Make phase
    # @param GenFdsTime      The total time of GenFds phase
    #
    def GenerateReport(self, BuildDuration, AutoGenTime, MakeTime, GenFdsTime):
        if self.ReportFile:
            try:
                File = []
                for (Wa, MaList) in self.ReportList:
                    PlatformReport(Wa, MaList, self.ReportType).GenerateReport(File, BuildDuration, AutoGenTime, MakeTime, GenFdsTime, self.ReportType)
                Content = FileLinesSplit(''.join(File), gLineMaxLength)
                SaveFileOnChange(self.ReportFile, Content, False)
                EdkLogger.quiet("Build report can be found at %s" % os.path.abspath(self.ReportFile))
            except IOError:
                EdkLogger.error(None, FILE_WRITE_FAILURE, ExtraData=self.ReportFile)
            except:
                EdkLogger.error("BuildReport", CODE_ERROR, "Unknown fatal error when generating build report", ExtraData=self.ReportFile, RaiseError=False)
                EdkLogger.quiet("(Python %s on %s\n%s)" % (platform.python_version(), sys.platform, traceback.format_exc()))

# This acts like the main() function for the script, unless it is 'import'ed into another script.
if __name__ == '__main__':
    pass

