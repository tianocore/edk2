## @file
# Routines for generating build report.
#
# This module contains the functionality to generate build report after
# build all target completes successfully.
#
# Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
from datetime import datetime
from StringIO import StringIO
from Common import EdkLogger
from Common.Misc import SaveFileOnChange
from Common.Misc import GuidStructureByteArrayToGuidString
from Common.Misc import GuidStructureStringToGuidString
from Common.InfClassObject import gComponentType2ModuleType
from Common.BuildToolError import FILE_WRITE_FAILURE
from Common.BuildToolError import CODE_ERROR
from Common.DataType import TAB_LINE_BREAK
from Common.DataType import TAB_DEPEX
from Common.DataType import TAB_SLASH
from Common.DataType import TAB_SPACE_SPLIT
from Common.DataType import TAB_BRG_PCD
from Common.DataType import TAB_BRG_LIBRARY
from Common.DataType import TAB_BACK_SLASH
from Common.LongFilePathSupport import OpenLongFilePath as open

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
gModulePattern = r"\n[-\w]+\s*\(([^,]+),\s*BaseAddress=%(Address)s,\s*EntryPoint=%(Address)s\)\s*\(GUID=([-0-9A-Fa-f]+)[^)]*\)"
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
gSectionStart = ">" + "=" * (gLineMaxLength-2) + "<"
gSectionEnd = "<" + "=" * (gLineMaxLength-2) + ">" + "\n"
gSectionSep = "=" * gLineMaxLength

## Tags for subsection start, end and separator
gSubSectionStart = ">" + "-" * (gLineMaxLength-2) + "<"
gSubSectionEnd = "<" + "-" * (gLineMaxLength-2) + ">"
gSubSectionSep = "-" * gLineMaxLength


## The look up table to map PCD type to pair of report display type and DEC type
gPcdTypeMap = {
  'FixedAtBuild'     : ('FIXED',  'FixedAtBuild'),
  'PatchableInModule': ('PATCH',  'PatchableInModule'),
  'FeatureFlag'      : ('FLAG',   'FeatureFlag'),
  'Dynamic'          : ('DYN',    'Dynamic'),
  'DynamicHii'       : ('DYNHII', 'Dynamic'),
  'DynamicVpd'       : ('DYNVPD', 'Dynamic'),
  'DynamicEx'        : ('DEX',    'DynamicEx'),
  'DynamicExHii'     : ('DEXHII', 'DynamicEx'),
  'DynamicExVpd'     : ('DEXVPD', 'DynamicEx'),
  }

## The look up table to map module type to driver type
gDriverTypeMap = {
  'SEC'               : '0x3 (SECURITY_CORE)',
  'PEI_CORE'          : '0x4 (PEI_CORE)',
  'PEIM'              : '0x6 (PEIM)',
  'DXE_CORE'          : '0x5 (DXE_CORE)',
  'DXE_DRIVER'        : '0x7 (DRIVER)',
  'DXE_SAL_DRIVER'    : '0x7 (DRIVER)',
  'DXE_SMM_DRIVER'    : '0x7 (DRIVER)',
  'DXE_RUNTIME_DRIVER': '0x7 (DRIVER)',
  'UEFI_DRIVER'       : '0x7 (DRIVER)',
  'UEFI_APPLICATION'  : '0x9 (APPLICATION)',
  'SMM_CORE'          : '0xD (SMM_CORE)',
  'SMM_DRIVER'        : '0xA (SMM)', # Extension of module type to support PI 1.1 SMM drivers
  }

## The look up table of the supported opcode in the dependency expression binaries
gOpCodeList = ["BEFORE", "AFTER", "PUSH", "AND", "OR", "NOT", "TRUE", "FALSE", "END", "SOR"]

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
    File.write(String + gEndOfLine)

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
        elif "GUID" in Type:
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
    
    NewContent = NewContent.replace(TAB_LINE_BREAK, gEndOfLine).replace('\r\r\n', gEndOfLine)
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
                            struct.unpack("=LHHBBBBBBBB", DepexFile.read(16))
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
        if int(str(M.AutoGenVersion), 0) >= 0x00010005:
            self._EdkIIModule = True
        else:
            self._EdkIIModule = False

        for Lib in M.DependentLibraryList:
            LibInfPath = str(Lib)
            LibClassList = Lib.LibraryClass[0].LibraryClass
            LibConstructorList = Lib.ConstructorList
            LibDesstructorList = Lib.DestructorList
            LibDepexList = Lib.DepexExpression[M.Arch, M.ModuleType]
            self.LibraryList.append((LibInfPath, LibClassList, LibConstructorList, LibDesstructorList, LibDepexList))

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
        FileWrite(File, gSubSectionStart)
        FileWrite(File, TAB_BRG_LIBRARY)
        if len(self.LibraryList) > 0:
            FileWrite(File, gSubSectionSep)
            for LibraryItem in self.LibraryList:
                LibInfPath = LibraryItem[0]
                FileWrite(File, LibInfPath)

                #
                # Report library class, library constructor and destructor for
                # EDKII style module.
                #
                if self._EdkIIModule:
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
            ModuleType = gComponentType2ModuleType.get(M.ComponentType, "")

        if ModuleType in ["SEC", "PEI_CORE", "DXE_CORE", "SMM_CORE", "UEFI_APPLICATION"]:
            return
      
        for Source in M.SourceFileList:
            if os.path.splitext(Source.Path)[1].lower() == ".dxs":
                Match = gDxsDependencyPattern.search(open(Source.Path).read())
                if Match:
                    self.Depex = Match.group(1).strip()
                    self.Source = "DXS"
                    break
        else:
            self.Depex = M.DepexExpressionList.get(M.ModuleType, "")
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
            FileWrite(File, gSubSectionStart)
            FileWrite(File, TAB_DEPEX)
            FileWrite(File, gSubSectionEnd)
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
            FileWrite(File, "%s" % self.Depex, True)
            FileWrite(File, gSubSectionSep)
            FileWrite(File, "From Module INF:  %s" % self.ModuleDepex, True)
            FileWrite(File, "From Library INF: %s" % self.LibraryDepex, True)
        else:
            FileWrite(File, "%s" % self.Depex)
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
        self.FileGuid = M.Guid
        self.Size = 0
        self.BuildTimeStamp = None
        self.DriverType = ""
        if not M.IsLibrary:
            ModuleType = M.ModuleType
            if not ModuleType:
                ModuleType = gComponentType2ModuleType.get(M.ComponentType, "")
            #
            # If a module complies to PI 1.1, promote Module type to "SMM_DRIVER"
            #
            if ModuleType == "DXE_SMM_DRIVER":
                PiSpec =  M.Module.Specification.get("PI_SPECIFICATION_VERSION", "0x00010000")
                if int(PiSpec, 0) >= 0x0001000A:
                    ModuleType = "SMM_DRIVER"
            self.DriverType = gDriverTypeMap.get(ModuleType, "0x2 (FREE_FORM)")
        self.UefiSpecVersion = M.Module.Specification.get("UEFI_SPECIFICATION_VERSION", "")
        self.PiSpecVersion = M.Module.Specification.get("PI_SPECIFICATION_VERSION", "")
        self.PciDeviceId = M.Module.Defines.get("PCI_DEVICE_ID", "")
        self.PciVendorId = M.Module.Defines.get("PCI_VENDOR_ID", "")
        self.PciClassCode = M.Module.Defines.get("PCI_CLASS_CODE", "")

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

        FwReportFileName = os.path.join(self._BuildDir, "DEBUG", self.ModuleName + ".txt")
        if os.path.isfile(FwReportFileName):
            try:
                FileContents = open(FwReportFileName).read()
                Match = gModuleSizePattern.search(FileContents)
                if Match:
                    self.Size = int(Match.group(1))

                Match = gTimeStampPattern.search(FileContents)
                if Match:
                    self.BuildTimeStamp = datetime.fromtimestamp(int(Match.group(1)))
            except IOError:
                EdkLogger.warn(None, "Fail to read report file", FwReportFileName)

        FileWrite(File, "Module Summary")
        FileWrite(File, "Module Name:          %s" % self.ModuleName)
        FileWrite(File, "Module INF Path:      %s" % self.ModuleInfPath)
        FileWrite(File, "File GUID:            %s" % self.FileGuid)
        if self.Size:
            FileWrite(File, "Size:                 0x%X (%.2fK)" % (self.Size, self.Size / 1024.0))
        if self.BuildTimeStamp:
            FileWrite(File, "Build Time Stamp:     %s" % self.BuildTimeStamp)
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
            GlobalPcdReport.GenerateReport(File, self.ModulePcdSet)

        if "LIBRARY" in ReportType:
            self.LibraryReport.GenerateReport(File)

        if "DEPEX" in ReportType:
            self.DepexReport.GenerateReport(File, GlobalDepexParser)

        if "BUILD_FLAGS" in ReportType:
            self.BuildFlagsReport.GenerateReport(File)

        if "FIXED_ADDRESS" in ReportType and self.FileGuid:
            GlobalPredictionReport.GenerateReport(File, self.FileGuid)

        FileWrite(File, gSectionEnd)

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
        self.MaxLen = 0
        if Wa.FdfProfile:
            self.FdfPcdSet = Wa.FdfProfile.PcdDict
        else:
            self.FdfPcdSet = {}

        self.ModulePcdOverride = {}
        for Pa in Wa.AutoGenObjectList:
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
        for Pa in Wa.AutoGenObjectList:
            for Package in Pa.PackageList:
                for (TokenCName, TokenSpaceGuidCName, DecType) in Package.Pcds:
                    DecDefaultValue = Package.Pcds[TokenCName, TokenSpaceGuidCName, DecType].DefaultValue
                    self.DecPcdDefault.setdefault((TokenCName, TokenSpaceGuidCName, DecType), DecDefaultValue)
        #
        # Collect PCDs defined in DSC common section
        #
        self.DscPcdDefault = {}
        for Arch in Wa.ArchList:
            Platform = Wa.BuildDatabase[Wa.MetaFile, Arch, Wa.BuildTarget, Wa.ToolChain]
            for (TokenCName, TokenSpaceGuidCName) in Platform.Pcds:
                DscDefaultValue = Platform.Pcds[(TokenCName, TokenSpaceGuidCName)].DefaultValue
                if DscDefaultValue:
                    self.DscPcdDefault[(TokenCName, TokenSpaceGuidCName)] = DscDefaultValue

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
    # @param DscOverridePcds Module DSC override PCDs set
    #
    def GenerateReport(self, File, ModulePcdSet):
        if ModulePcdSet == None:
            #
            # For platform global PCD section
            #
            FileWrite(File, gSectionStart)
            FileWrite(File, "Platform Configuration Database Report")
            FileWrite(File, "  *P  - Platform scoped PCD override in DSC file")
            FileWrite(File, "  *F  - Platform scoped PCD override in FDF file")
            FileWrite(File, "  *M  - Module scoped PCD override")
            FileWrite(File, gSectionSep)
        else:
            #
            # For module PCD sub-section
            #
            FileWrite(File, gSubSectionStart)
            FileWrite(File, TAB_BRG_PCD)
            FileWrite(File, gSubSectionSep)

        for Key in self.AllPcds:
            #
            # Group PCD by their token space GUID C Name
            #
            First = True
            for Type in self.AllPcds[Key]:
                #
                # Group PCD by their usage type
                #
                TypeName, DecType = gPcdTypeMap.get(Type, ("", Type))
                for Pcd in self.AllPcds[Key][Type]:
                    #
                    # Get PCD default value and their override relationship
                    #
                    DecDefaultValue = self.DecPcdDefault.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, DecType))
                    DscDefaultValue = self.DscPcdDefault.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName))
                    DscDefaultValue = self.FdfPcdSet.get((Pcd.TokenCName, Key), DscDefaultValue)
                    InfDefaultValue = None
                    
                    PcdValue = DecDefaultValue
                    if DscDefaultValue:
                        PcdValue = DscDefaultValue
                    if ModulePcdSet != None:
                        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Type) not in ModulePcdSet:
                            continue
                        InfDefault, PcdValue = ModulePcdSet[Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Type]
                        if InfDefault == "":
                            InfDefault = None
                    if First:
                        if ModulePcdSet == None:
                            FileWrite(File, "")
                        FileWrite(File, Key)
                        First = False


                    if Pcd.DatumType in ('UINT8', 'UINT16', 'UINT32', 'UINT64'):
                        PcdValueNumber = int(PcdValue.strip(), 0)
                        if DecDefaultValue == None:
                            DecMatch = True
                        else:
                            DecDefaultValueNumber = int(DecDefaultValue.strip(), 0)
                            DecMatch = (DecDefaultValueNumber == PcdValueNumber)

                        if InfDefaultValue == None:
                            InfMatch = True
                        else:
                            InfDefaultValueNumber = int(InfDefaultValue.strip(), 0)
                            InfMatch = (InfDefaultValueNumber == PcdValueNumber)

                        if DscDefaultValue == None:
                            DscMatch = True
                        else:
                            DscDefaultValueNumber = int(DscDefaultValue.strip(), 0)
                            DscMatch = (DscDefaultValueNumber == PcdValueNumber)
                    else:
                        if DecDefaultValue == None:
                            DecMatch = True
                        else:
                            DecMatch = (DecDefaultValue.strip() == PcdValue.strip())

                        if InfDefaultValue == None:
                            InfMatch = True
                        else:
                            InfMatch = (InfDefaultValue.strip() == PcdValue.strip())

                        if DscDefaultValue == None:
                            DscMatch = True
                        else:
                            DscMatch = (DscDefaultValue.strip() == PcdValue.strip())

                    #
                    # Report PCD item according to their override relationship
                    #
                    if DecMatch and InfMatch:
                        FileWrite(File, '    %-*s: %6s %10s = %-22s' % (self.MaxLen, Pcd.TokenCName, TypeName, '('+Pcd.DatumType+')', PcdValue.strip()))
                    else:
                        if DscMatch:
                            if (Pcd.TokenCName, Key) in self.FdfPcdSet:
                                FileWrite(File, ' *F %-*s: %6s %10s = %-22s' % (self.MaxLen, Pcd.TokenCName, TypeName, '('+Pcd.DatumType+')', PcdValue.strip()))
                            else:
                                FileWrite(File, ' *P %-*s: %6s %10s = %-22s' % (self.MaxLen, Pcd.TokenCName, TypeName, '('+Pcd.DatumType+')', PcdValue.strip()))
                        else:
                            FileWrite(File, ' *M %-*s: %6s %10s = %-22s' % (self.MaxLen, Pcd.TokenCName, TypeName, '('+Pcd.DatumType+')', PcdValue.strip()))
                    
                    if TypeName in ('DYNHII', 'DEXHII', 'DYNVPD', 'DEXVPD'):
                        for SkuInfo in Pcd.SkuInfoList.values():
                            if TypeName in ('DYNHII', 'DEXHII'):
                                FileWrite(File, '%*s: %s: %s' % (self.MaxLen + 4, SkuInfo.VariableGuid, SkuInfo.VariableName, SkuInfo.VariableOffset))        
                            else:
                                FileWrite(File, '%*s' % (self.MaxLen + 4, SkuInfo.VpdOffset))
                               
                    if not DscMatch and DscDefaultValue != None:
                        FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'DSC DEFAULT', DscDefaultValue.strip()))

                    if not InfMatch and InfDefaultValue != None:
                        FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'INF DEFAULT', InfDefaultValue.strip()))

                    if not DecMatch and DecDefaultValue != None:
                        FileWrite(File, '    %*s = %s' % (self.MaxLen + 19, 'DEC DEFAULT', DecDefaultValue.strip()))

                    if ModulePcdSet == None:
                        ModuleOverride = self.ModulePcdOverride.get((Pcd.TokenCName, Pcd.TokenSpaceGuidCName), {})
                        for ModulePath in ModuleOverride:
                            ModuleDefault = ModuleOverride[ModulePath]
                            if Pcd.DatumType in ('UINT8', 'UINT16', 'UINT32', 'UINT64'):
                                ModulePcdDefaultValueNumber = int(ModuleDefault.strip(), 0)
                                Match = (ModulePcdDefaultValueNumber == PcdValueNumber)
                            else:
                                Match = (ModuleDefault.strip() == PcdValue.strip())
                            if Match:
                                continue
                            FileWrite(File, ' *M %-*s = %s' % (self.MaxLen + 19, ModulePath, ModuleDefault.strip()))

        if ModulePcdSet == None:
            FileWrite(File, gSectionEnd)
        else:
            FileWrite(File, gSubSectionEnd)



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
                if Module.ModuleType == "BASE":
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
                    if int(str(Module.AutoGenVersion), 0) >= 0x00010005:
                        RealEntryPoint = "_ModuleEntryPoint"
                    else:
                        RealEntryPoint = EntryPoint
                        if EntryPoint == "_ModuleEntryPoint":
                            CCFlags = Module.BuildOption.get("CC", {}).get("FLAGS", "")
                            Match = gGlueLibEntryPoint.search(CCFlags)
                            if Match:
                                EntryPoint = Match.group(1)

                    self._FfsEntryPoint[Module.Guid.upper()] = (EntryPoint, RealEntryPoint)


        #
        # Collect platform firmware volume list as the input of EOT.
        #
        self._FvList = []
        if Wa.FdfProfile:
            for Fd in Wa.FdfProfile.FdDict:
                for FdRegion in Wa.FdfProfile.FdDict[Fd].RegionList:
                    if FdRegion.RegionType != "FV":
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

        TempFile = open(SourceList, "w+")
        for Item in self._SourceList:
            FileWrite(TempFile, Item)
        TempFile.close()
        TempFile = open(GuidList, "w+")
        for Key in self._GuidMap:
            FileWrite(TempFile, "%s %s" % (Key, self._GuidMap[Key]))
        TempFile.close()

        try:
            from Eot.Eot import Eot

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

        #
        # If the input FdRegion is not a firmware volume,
        # we are done.
        #
        if self.Type != "FV":
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
        # Collect PCDs defined in DSC common section
        #
        Platform = Wa.BuildDatabase[Wa.MetaFile, 'COMMON']
        for (TokenCName, TokenSpaceGuidCName) in Platform.Pcds:
            DscDefaultValue = Platform.Pcds[(TokenCName, TokenSpaceGuidCName)].DefaultValue
            PlatformPcds[(TokenCName, TokenSpaceGuidCName)] = DscDefaultValue

        #
        # Add PEI and DXE a priori files GUIDs defined in PI specification.
        #
        self._GuidsDb["1B45CC0A-156A-428A-AF62-49864DA0E6E6"] = "PEI Apriori"
        self._GuidsDb["FC510EE7-FFDC-11D4-BD41-0080C73C8881"] = "DXE Apriori"
        #
        # Add ACPI table storage file
        #
        self._GuidsDb["7E374E25-8E01-4FEE-87F2-390C23C606CD"] = "ACPI table storage"

        for Pa in Wa.AutoGenObjectList:
            for ModuleKey in Pa.Platform.Modules:
                M = Pa.Platform.Modules[ModuleKey].M
                InfPath = os.path.join(Wa.WorkspaceDir, M.MetaFile.File)
                self._GuidsDb[M.Guid.upper()] = "%s (%s)" % (M.Module.BaseName, InfPath)

        #
        # Collect the GUID map in the FV firmware volume
        #
        for FvName in self.FvList:
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
                            ModuleSectFile = os.path.join(Wa.WorkspaceDir, Section.SectFileName)
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

        if self.Type == "FV":
            FvTotalSize = 0
            FvTakenSize = 0
            FvFreeSize  = 0
            FvReportFileName = os.path.join(self._FvDir, FvName + ".Fv.txt")
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
                OffsetList = OffsetInfo.keys()
                OffsetList.sort()
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
                self._GenerateReport(File, Info[0], "FV", Info[1], Info[2], FvItem)
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
        if "FLASH" in ReportType and Wa.FdfProfile and MaList == None:
            for Fd in Wa.FdfProfile.FdDict:
                self.FdReportList.append(FdReport(Wa.FdfProfile.FdDict[Fd], Wa))

        self.PredictionReport = None
        if "FIXED_ADDRESS" in ReportType or "EXECUTION_ORDER" in ReportType:
            self.PredictionReport = PredictionReport(Wa)

        self.DepexParser = None
        if "DEPEX" in ReportType:
            self.DepexParser = DepexParser(Wa)
            
        self.ModuleReportList = []
        if MaList != None:
            self._IsModuleBuild = True
            for Ma in MaList:
                self.ModuleReportList.append(ModuleReport(Ma, ReportType))
        else:
            self._IsModuleBuild = False
            for Pa in Wa.AutoGenObjectList:
                for ModuleKey in Pa.Platform.Modules:
                    self.ModuleReportList.append(ModuleReport(Pa.Platform.Modules[ModuleKey].M, ReportType))



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
    # @param ReportType      The kind of report items in the final report file
    #
    def GenerateReport(self, File, BuildDuration, ReportType):
        FileWrite(File, "Platform Summary")
        FileWrite(File, "Platform Name:        %s" % self.PlatformName)
        FileWrite(File, "Platform DSC Path:    %s" % self.PlatformDscPath)
        FileWrite(File, "Architectures:        %s" % self.Architectures)
        FileWrite(File, "Tool Chain:           %s" % self.ToolChain)
        FileWrite(File, "Target:               %s" % self.Target)
        FileWrite(File, "Output Path:          %s" % self.OutputPath)
        FileWrite(File, "Build Environment:    %s" % self.BuildEnvironment)
        FileWrite(File, "Build Duration:       %s" % BuildDuration)
        FileWrite(File, "Report Content:       %s" % ", ".join(ReportType))

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
                self.ReportType = ["PCD", "LIBRARY", "BUILD_FLAGS", "DEPEX", "FLASH", "FIXED_ADDRESS"]
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
    #
    def GenerateReport(self, BuildDuration):
        if self.ReportFile:
            try:
                File = StringIO('')
                for (Wa, MaList) in self.ReportList:
                    PlatformReport(Wa, MaList, self.ReportType).GenerateReport(File, BuildDuration, self.ReportType)
                Content = FileLinesSplit(File.getvalue(), gLineMaxLength)
                SaveFileOnChange(self.ReportFile, Content, True)
                EdkLogger.quiet("Build report can be found at %s" % os.path.abspath(self.ReportFile))
            except IOError:
                EdkLogger.error(None, FILE_WRITE_FAILURE, ExtraData=self.ReportFile)
            except:
                EdkLogger.error("BuildReport", CODE_ERROR, "Unknown fatal error when generating build report", ExtraData=self.ReportFile, RaiseError=False)
                EdkLogger.quiet("(Python %s on %s\n%s)" % (platform.python_version(), sys.platform, traceback.format_exc()))
            File.close()
            
# This acts like the main() function for the script, unless it is 'import'ed into another script.
if __name__ == '__main__':
    pass

