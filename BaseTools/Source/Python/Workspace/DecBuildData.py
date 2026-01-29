## @file
# This file is used to create a database used by build tool
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from Common.StringUtils import *
from Common.DataType import *
from Common.Misc import *
from types import *
from collections import OrderedDict
from CommonDataClass.DataClass import *
from Workspace.BuildClassObject import PackageBuildClassObject, StructurePcd, PcdClassObject
from Common.GlobalData import gGlobalDefines
from re import compile

## Platform build information from DEC file
#
#  This class is used to retrieve information stored in database and convert them
# into PackageBuildClassObject form for easier use for AutoGen.
#
class DecBuildData(PackageBuildClassObject):

    # dict used to convert part of [Defines] to members of DecBuildData directly
    _PROPERTY_ = {
        #
        # Required Fields
        #
        TAB_DEC_DEFINES_PACKAGE_NAME                : "_PackageName",
        TAB_DEC_DEFINES_PACKAGE_GUID                : "_Guid",
        TAB_DEC_DEFINES_PACKAGE_VERSION             : "_Version",
        TAB_DEC_DEFINES_PKG_UNI_FILE                : "_PkgUniFile",
    }

    ## Constructor of DecBuildData
    #
    #  Initialize object of DecBuildData
    #
    #   @param      FilePath        The path of package description file
    #   @param      RawData         The raw data of DEC file
    #   @param      BuildDataBase   Database used to retrieve module information
    #   @param      Arch            The target architecture
    #   @param      Platform        (not used for DecBuildData)
    #   @param      Macros          Macros used for replacement in DSC file
    #
    def __init__(self, File, RawData, BuildDataBase, Arch=TAB_ARCH_COMMON, Target=None, Toolchain=None):
        self.MetaFile = File
        self._PackageDir = File.Dir
        self._RawData = RawData
        self._Bdb = BuildDataBase
        self._Arch = Arch
        self._Target = Target
        self._Toolchain = Toolchain
        self._Clear()
        self.UpdatePcdTypeDict()

    ## XXX[key] = value
    def __setitem__(self, key, value):
        self.__dict__[self._PROPERTY_[key]] = value

    ## value = XXX[key]
    def __getitem__(self, key):
        return self.__dict__[self._PROPERTY_[key]]

    ## "in" test support
    def __contains__(self, key):
        return key in self._PROPERTY_

    ## Set all internal used members of DecBuildData to None
    def _Clear(self):
        self._Header            = None
        self._PackageName       = None
        self._Guid              = None
        self._Version           = None
        self._PkgUniFile        = None
        self._Protocols         = None
        self._Ppis              = None
        self._Guids             = None
        self._Includes          = None
        self._CommonIncludes    = None
        self._LibraryClasses    = None
        self._Pcds              = None
        self._MacroDict         = None
        self._PrivateProtocols  = None
        self._PrivatePpis       = None
        self._PrivateGuids      = None
        self._PrivateIncludes   = None

    ## Get current effective macros
    @property
    def _Macros(self):
        if self._MacroDict is None:
            self._MacroDict = dict(gGlobalDefines)
        return self._MacroDict

    ## Get architecture
    @property
    def Arch(self):
        return self._Arch

    ## Retrieve all information in [Defines] section
    #
    #   (Retrieving all [Defines] information in one-shot is just to save time.)
    #
    def _GetHeaderInfo(self):
        RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch]
        for Record in RecordList:
            Name = Record[1]
            if Name in self:
                self[Name] = Record[2]
        self._Header = 'DUMMY'

    ## Retrieve package name
    @property
    def PackageName(self):
        if self._PackageName is None:
            if self._Header is None:
                self._GetHeaderInfo()
            if self._PackageName is None:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "No PACKAGE_NAME", File=self.MetaFile)
        return self._PackageName

    ## Retrieve file guid
    @property
    def PackageName(self):
        if self._Guid is None:
            if self._Header is None:
                self._GetHeaderInfo()
            if self._Guid is None:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "No PACKAGE_GUID", File=self.MetaFile)
        return self._Guid

    ## Retrieve package version
    @property
    def Version(self):
        if self._Version is None:
            if self._Header is None:
                self._GetHeaderInfo()
            if self._Version is None:
                self._Version = ''
        return self._Version

    ## Retrieve protocol definitions (name/value pairs)
    @property
    def Protocols(self):
        if self._Protocols is None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # protocol definition for given ARCH
            #
            ProtocolDict = tdict(True)
            PrivateProtocolDict = tdict(True)
            NameList = []
            PrivateNameList = []
            PublicNameList = []
            # find out all protocol definitions for specific and 'common' arch
            RecordList = self._RawData[MODEL_EFI_PROTOCOL, self._Arch]
            for Name, Guid, Dummy, Arch, PrivateFlag, ID, LineNo in RecordList:
                if PrivateFlag == 'PRIVATE':
                    if Name not in PrivateNameList:
                        PrivateNameList.append(Name)
                        PrivateProtocolDict[Arch, Name] = Guid
                    if Name in PublicNameList:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % Name, File=self.MetaFile, Line=LineNo)
                else:
                    if Name not in PublicNameList:
                        PublicNameList.append(Name)
                    if Name in PrivateNameList:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % Name, File=self.MetaFile, Line=LineNo)
                if Name not in NameList:
                    NameList.append(Name)
                ProtocolDict[Arch, Name] = Guid
            # use OrderedDict to keep the order
            self._Protocols = OrderedDict()
            self._PrivateProtocols = OrderedDict()
            for Name in NameList:
                #
                # limit the ARCH to self._Arch, if no self._Arch found, tdict
                # will automatically turn to 'common' ARCH for trying
                #
                self._Protocols[Name] = ProtocolDict[self._Arch, Name]
            for Name in PrivateNameList:
                self._PrivateProtocols[Name] = PrivateProtocolDict[self._Arch, Name]
        return self._Protocols

    ## Retrieve PPI definitions (name/value pairs)
    @property
    def Ppis(self):
        if self._Ppis is None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # PPI definition for given ARCH
            #
            PpiDict = tdict(True)
            PrivatePpiDict = tdict(True)
            NameList = []
            PrivateNameList = []
            PublicNameList = []
            # find out all PPI definitions for specific arch and 'common' arch
            RecordList = self._RawData[MODEL_EFI_PPI, self._Arch]
            for Name, Guid, Dummy, Arch, PrivateFlag, ID, LineNo in RecordList:
                if PrivateFlag == 'PRIVATE':
                    if Name not in PrivateNameList:
                        PrivateNameList.append(Name)
                        PrivatePpiDict[Arch, Name] = Guid
                    if Name in PublicNameList:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % Name, File=self.MetaFile, Line=LineNo)
                else:
                    if Name not in PublicNameList:
                        PublicNameList.append(Name)
                    if Name in PrivateNameList:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % Name, File=self.MetaFile, Line=LineNo)
                if Name not in NameList:
                    NameList.append(Name)
                PpiDict[Arch, Name] = Guid
            # use OrderedDict to keep the order
            self._Ppis = OrderedDict()
            self._PrivatePpis = OrderedDict()
            for Name in NameList:
                #
                # limit the ARCH to self._Arch, if no self._Arch found, tdict
                # will automatically turn to 'common' ARCH for trying
                #
                self._Ppis[Name] = PpiDict[self._Arch, Name]
            for Name in PrivateNameList:
                self._PrivatePpis[Name] = PrivatePpiDict[self._Arch, Name]
        return self._Ppis

    ## Retrieve GUID definitions (name/value pairs)
    @property
    def Guids(self):
        if self._Guids is None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # GUID definition for given ARCH
            #
            GuidDict = tdict(True)
            PrivateGuidDict = tdict(True)
            NameList = []
            PrivateNameList = []
            PublicNameList = []
            # find out all protocol definitions for specific and 'common' arch
            RecordList = self._RawData[MODEL_EFI_GUID, self._Arch]
            for Name, Guid, Dummy, Arch, PrivateFlag, ID, LineNo in RecordList:
                if PrivateFlag == 'PRIVATE':
                    if Name not in PrivateNameList:
                        PrivateNameList.append(Name)
                        PrivateGuidDict[Arch, Name] = Guid
                    if Name in PublicNameList:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % Name, File=self.MetaFile, Line=LineNo)
                else:
                    if Name not in PublicNameList:
                        PublicNameList.append(Name)
                    if Name in PrivateNameList:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % Name, File=self.MetaFile, Line=LineNo)
                if Name not in NameList:
                    NameList.append(Name)
                GuidDict[Arch, Name] = Guid
            # use OrderedDict to keep the order
            self._Guids = OrderedDict()
            self._PrivateGuids = OrderedDict()
            for Name in NameList:
                #
                # limit the ARCH to self._Arch, if no self._Arch found, tdict
                # will automatically turn to 'common' ARCH for trying
                #
                self._Guids[Name] = GuidDict[self._Arch, Name]
            for Name in PrivateNameList:
                self._PrivateGuids[Name] = PrivateGuidDict[self._Arch, Name]
        return self._Guids

    ## Retrieve public include paths declared in this package
    @property
    def Includes(self):
        if self._Includes is None or self._CommonIncludes is None:
            self._CommonIncludes = []
            self._Includes = []
            self._PrivateIncludes = []
            PublicInclues = []
            RecordList = self._RawData[MODEL_EFI_INCLUDE, self._Arch]
            Macros = self._Macros
            for Record in RecordList:
                File = PathClass(NormPath(Record[0], Macros), self._PackageDir, Arch=self._Arch)
                LineNo = Record[-1]
                # validate the path
                ErrorCode, ErrorInfo = File.Validate()
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)

                # avoid duplicate include path
                if File not in self._Includes:
                    self._Includes.append(File)
                if Record[4] == 'PRIVATE':
                    if File not in self._PrivateIncludes:
                        self._PrivateIncludes.append(File)
                    if File in PublicInclues:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % File, File=self.MetaFile, Line=LineNo)
                else:
                    if File not in PublicInclues:
                        PublicInclues.append(File)
                    if File in self._PrivateIncludes:
                        EdkLogger.error('build', OPTION_CONFLICT, "Can't determine %s's attribute, it is both defined as Private and non-Private attribute in DEC file." % File, File=self.MetaFile, Line=LineNo)
                if Record[3] == TAB_COMMON:
                    self._CommonIncludes.append(File)
        return self._Includes

    ## Retrieve library class declarations (not used in build at present)
    @property
    def LibraryClasses(self):
        if self._LibraryClasses is None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # library class declaration for given ARCH
            #
            LibraryClassDict = tdict(True)
            LibraryClassSet = set()
            RecordList = self._RawData[MODEL_EFI_LIBRARY_CLASS, self._Arch]
            Macros = self._Macros
            for LibraryClass, File, Dummy, Arch, PrivateFlag, ID, LineNo in RecordList:
                File = PathClass(NormPath(File, Macros), self._PackageDir, Arch=self._Arch)
                # check the file validation
                ErrorCode, ErrorInfo = File.Validate()
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)
                LibraryClassSet.add(LibraryClass)
                LibraryClassDict[Arch, LibraryClass] = File
            self._LibraryClasses = OrderedDict()
            for LibraryClass in LibraryClassSet:
                self._LibraryClasses[LibraryClass] = LibraryClassDict[self._Arch, LibraryClass]
        return self._LibraryClasses

    ## Retrieve PCD declarations
    @property
    def Pcds(self):
        if self._Pcds is None:
            self._Pcds = OrderedDict()
            self._Pcds.update(self._GetPcd(MODEL_PCD_FIXED_AT_BUILD))
            self._Pcds.update(self._GetPcd(MODEL_PCD_PATCHABLE_IN_MODULE))
            self._Pcds.update(self._GetPcd(MODEL_PCD_FEATURE_FLAG))
            self._Pcds.update(self._GetPcd(MODEL_PCD_DYNAMIC))
            self._Pcds.update(self._GetPcd(MODEL_PCD_DYNAMIC_EX))
        return self._Pcds

    def ParsePcdName(self,TokenCName):
        TokenCName = TokenCName.strip()
        if TokenCName.startswith("["):
            if "." in TokenCName:
                Demesionattr = TokenCName[:TokenCName.index(".")]
                Fields = TokenCName[TokenCName.index(".")+1:]
            else:
                Demesionattr = TokenCName
                Fields = ""
        else:
            Demesionattr = ""
            Fields = TokenCName

        return Demesionattr,Fields

    def ProcessStructurePcd(self, StructurePcdRawDataSet):
        s_pcd_set = OrderedDict()
        for s_pcd, LineNo in StructurePcdRawDataSet:
            if s_pcd.TokenSpaceGuidCName not in s_pcd_set:
                s_pcd_set[s_pcd.TokenSpaceGuidCName] = []
            s_pcd_set[s_pcd.TokenSpaceGuidCName].append((s_pcd, LineNo))

        str_pcd_set = []
        for pcdname in s_pcd_set:
            dep_pkgs = []
            struct_pcd = StructurePcd()
            for item, LineNo in s_pcd_set[pcdname]:
                if not item.TokenCName:
                    continue
                if "<HeaderFiles>" in item.TokenCName:
                    struct_pcd.StructuredPcdIncludeFile.append(item.DefaultValue)
                elif "<Packages>" in item.TokenCName:
                    dep_pkgs.append(item.DefaultValue)
                elif item.DatumType == item.TokenCName:
                    struct_pcd.copy(item)
                    struct_pcd.TokenValue = struct_pcd.TokenValue.strip("{").strip()
                    struct_pcd.TokenSpaceGuidCName, struct_pcd.TokenCName = pcdname.split(".")
                    struct_pcd.PcdDefineLineNo = LineNo
                    struct_pcd.PkgPath = self.MetaFile.File
                    struct_pcd.SetDecDefaultValue(item.DefaultValue,self.MetaFile.File,LineNo)
                else:
                    DemesionAttr, Fields = self.ParsePcdName(item.TokenCName)
                    struct_pcd.AddDefaultValue(Fields, item.DefaultValue, self.MetaFile.File, LineNo,DemesionAttr)

            struct_pcd.PackageDecs = dep_pkgs
            str_pcd_set.append(struct_pcd)
        return str_pcd_set

    ## Retrieve PCD declarations for given type
    def _GetPcd(self, Type):
        Pcds = OrderedDict()
        #
        # tdict is a special kind of dict, used for selecting correct
        # PCD declaration for given ARCH
        #
        PcdDict = tdict(True, 3)
        # for summarizing PCD
        PcdSet = []
        # find out all PCDs of the 'type'

        StrPcdSet = []
        RecordList = self._RawData[Type, self._Arch]
        for TokenSpaceGuid, PcdCName, Setting, Arch, PrivateFlag, Dummy1, Dummy2 in RecordList:
            PcdDict[Arch, PcdCName, TokenSpaceGuid] = (Setting, Dummy2)
            if not (PcdCName, TokenSpaceGuid) in PcdSet:
                PcdSet.append((PcdCName, TokenSpaceGuid))

        DefinitionPosition = {}
        for PcdCName, TokenSpaceGuid in PcdSet:
            #
            # limit the ARCH to self._Arch, if no self._Arch found, tdict
            # will automatically turn to 'common' ARCH and try again
            #
            Setting, LineNo = PcdDict[self._Arch, PcdCName, TokenSpaceGuid]
            if Setting is None:
                continue

            DefaultValue, DatumType, TokenNumber = AnalyzePcdData(Setting)
            validateranges, validlists, expressions = self._RawData.GetValidExpression(TokenSpaceGuid, PcdCName)
            PcdObj = PcdClassObject(
                                        PcdCName,
                                        TokenSpaceGuid,
                                        self._PCD_TYPE_STRING_[Type],
                                        DatumType,
                                        DefaultValue,
                                        TokenNumber,
                                        '',
                                        {},
                                        False,
                                        None,
                                        list(validateranges),
                                        list(validlists),
                                        list(expressions)
                                        )
            DefinitionPosition[PcdObj] = (self.MetaFile.File, LineNo)
            if "." in TokenSpaceGuid:
                StrPcdSet.append((PcdObj, LineNo))
            else:
                Pcds[PcdCName, TokenSpaceGuid, self._PCD_TYPE_STRING_[Type]] = PcdObj

        StructurePcds = self.ProcessStructurePcd(StrPcdSet)
        for pcd in StructurePcds:
            Pcds[pcd.TokenCName, pcd.TokenSpaceGuidCName, self._PCD_TYPE_STRING_[Type]] = pcd
        StructPattern = compile(r'[_a-zA-Z][0-9A-Za-z_]*$')
        for pcd in Pcds.values():
            if pcd.DatumType not in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64, TAB_VOID, "BOOLEAN"]:
                if not pcd.IsAggregateDatumType():
                    EdkLogger.error('build', FORMAT_INVALID, "DatumType only support BOOLEAN, UINT8, UINT16, UINT32, UINT64, VOID* or a valid struct name.", DefinitionPosition[pcd][0], DefinitionPosition[pcd][1])
                elif not pcd.IsArray() and not pcd.StructuredPcdIncludeFile:
                    EdkLogger.error("build", PCD_STRUCTURE_PCD_ERROR, "The structure Pcd %s.%s header file is not found in %s line %s \n" % (pcd.TokenSpaceGuidCName, pcd.TokenCName, pcd.DefinitionPosition[0], pcd.DefinitionPosition[1] ))
        return Pcds

    @property
    def CommonIncludes(self):
        if self._CommonIncludes is None:
            self.Includes
        return self._CommonIncludes
