## @file InfPomAlignment.py
# This file contained the adapter for convert INF parser object to POM Object
#
# Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
'''
InfPomAlignment
'''
##
# Import modules
#
import os.path
from Logger import StringTable as ST
import Logger.Log as Logger
from Library.String import FORMAT_INVALID
from Library.String import PARSER_ERROR
from Library.String import NormPath
from Library.String import GetSplitValueList
from Library.Misc import ConvertVersionToDecimal
from Library.Misc import GetHelpStringByRemoveHashKey
from Library.Misc import ConvertArchList
from Library.Misc import GetRelativePath
from Library.Misc import PathClass
from Library.Parsing import GetPkgInfoFromDec
from Library.UniClassObject import UniFileClassObject
from Library.UniClassObject import ConvertSpecialUnicodes
from Library.UniClassObject import GetLanguageCode1766
from Library import DataType as DT
from Library import GlobalData
from Library.ParserValidate import IsValidPath
from Object.POM import CommonObject
from Object.POM.ModuleObject import ModuleObject
from Object.POM.ModuleObject import ExternObject
from Object.POM.ModuleObject import HobObject
from Object.POM.ModuleObject import EventObject
from Object.POM.ModuleObject import BootModeObject
from Object.POM.ModuleObject import PackageDependencyObject
from Object.POM.ModuleObject import SourceFileObject
from Object.POM.ModuleObject import DepexObject
from Object.POM.ModuleObject import AsBuildLibraryClassObject
from Object.POM.ModuleObject import AsBuiltObject
from PomAdapter.InfPomAlignmentMisc import GenModuleHeaderUserExt
from PomAdapter.InfPomAlignmentMisc import GenBinaryData
from Parser import InfParser
from PomAdapter.DecPomAlignment import DecPomAlignment


## InfPomAlignment
#
# Inherit from ModuleObject
#
class InfPomAlignment(ModuleObject):
    ## Construct of InfPomAlignment
    # Skip means that UPT don't care the syntax of INF, this may be the not 
    # distributed INF files during creation or the INF files checked for 
    # dependency rule during remove.
    #
    def __init__(self, FileName, WorkSpace=None, PackagePath='', Skip=False):
        ModuleObject.__init__(self)
        self.Parser = None
        self.FileName = FileName
        self.WorkSpace = WorkSpace
        self.CombinePath = ''
        self.LibModuleTypeList = []
        self.FullPath = ''
        self.ModulePath = ''
        self.WorkspaceDir = " "
        self.CustomMakefile = []
        self.UniFileClassObject = None
        self.SetPackagePath(PackagePath)
        #
        # Call GenInfPomObjects function to fill POM object.
        #
        if Skip:
            OrigConfig = Logger.SUPRESS_ERROR
            Logger.SUPRESS_ERROR = True
            try:
                self._GenInfPomObjects(Skip)
            finally:
                Logger.SUPRESS_ERROR = OrigConfig
        else:
            self._GenInfPomObjects(Skip)

    ##
    # Generate all POM objects, the original input comes 
    # from INF parser's output
    #
    def _GenInfPomObjects(self, Skip):
        #
        # Call INF Parser to get information from INF file  
        #
        self.Parser = InfParser.InfParser(self.FileName, self.WorkSpace)
        self.FullPath = self.Parser.FullPath
        self.GetFullPath()
        self._GenModuleHeader()
        #
        # Call GenBinaries after Module Header for Binary INF consideration.
        #
        self._GenBinaries()
        self._GenBuildOptions()
        self._GenLibraryClasses()
        self._GenPackages(Skip)
        self._GenPcds()
        self._GenSources()
        self._GenUserExtensions()
        self._GenGuidProtocolPpis(DT.TAB_GUIDS)
        self._GenGuidProtocolPpis(DT.TAB_PROTOCOLS)
        self._GenGuidProtocolPpis(DT.TAB_PPIS)
        self._GenDepexes()

    ## Convert [Defines] section content to InfDefObject
    #
    # Convert [Defines] section content to InfDefObject
    #
    # @param Defines        The content under [Defines] section
    # @param ModuleHeader   An object of ModuleHeaderClass
    # @param Arch           The supported ARCH
    #
    def _GenModuleHeader(self):
        Logger.Debug(2, "Generate ModuleHeader ...")
        #
        # Get all defines information form InfParser Object
        #
        RecordSet = self.Parser.InfDefSection.Defines
        #
        # Should only have one ArchString Item.
        #
        ArchString = RecordSet.keys()[0]
        ArchList = GetSplitValueList(ArchString, ' ')
        ArchList = ConvertArchList(ArchList)
        HasCalledFlag = False
        #
        # Get data from Sdict()
        #
        ValueList = RecordSet[ArchString]
        self.SetFileName(self.FileName)
        self.SetFullPath(self.FullPath)
        #
        # The INF's filename (without the directory path or the extension) 
        # must be used for the value of the 
        # ModuleSurfaceArea.Header.Name element
        #
        self.SetName(os.path.splitext(os.path.basename(self.FileName))[0])
        self.WorkspaceDir = " "
        #
        # CombinePath and ModulePath
        #
        CombinePath = GetRelativePath(self.FullPath, self.WorkSpace)
        self.SetCombinePath(CombinePath)
        ModulePath = os.path.split(CombinePath)[0]
        ModuleRelativePath = ModulePath
        if self.GetPackagePath() != '':
            ModuleRelativePath = GetRelativePath(ModulePath, self.GetPackagePath())
        self.SetModulePath(ModuleRelativePath)
        #
        # For Define Seciton Items.
        #
        DefineObj = ValueList
        #
        # Convert UEFI/PI version to decimal number
        #
        if DefineObj.GetUefiSpecificationVersion() != None:
            __UefiVersion = DefineObj.GetUefiSpecificationVersion().GetValue()
            __UefiVersion = ConvertVersionToDecimal(__UefiVersion)
            self.SetUefiSpecificationVersion(str(__UefiVersion))
        if DefineObj.GetPiSpecificationVersion() != None:
            __PiVersion = DefineObj.GetPiSpecificationVersion().GetValue()
            __PiVersion = ConvertVersionToDecimal(__PiVersion)

            self.SetPiSpecificationVersion(str(__PiVersion))
        SpecList = DefineObj.GetSpecification()
        NewSpecList = []
        for SpecItem in SpecList:
            NewSpecList.append((SpecItem[0], ConvertVersionToDecimal(SpecItem[1])))
        self.SetSpecList(NewSpecList)

        #
        # must exist items in INF define section
        # MODULE_TYPE/BASE_NAME/INF_VERSION/FILE_GUID/VERSION_STRING 
        #
        if DefineObj.GetModuleType() == None:
            Logger.Error("InfParser", FORMAT_INVALID,
                         ST.ERR_INF_PARSER_DEFINE_SECTION_MUST_ITEM_NOT_EXIST % ("MODULE_TYPE"), File=self.FullPath)
        else:
            self.SetModuleType(DefineObj.GetModuleType().GetValue())
            ModuleType = DefineObj.GetModuleType().GetValue()
            if ModuleType:
                #
                # Drivers and applications are not allowed to have a MODULE_TYPE of "BASE". Only 
                # libraries are permitted to a have a MODULE_TYPE of "BASE".
                #
                if len(DefineObj.LibraryClass) == 0 and ModuleType == 'BASE':
                    Logger.Error("InfParser",
                                 FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_MODULETYPE_INVALID,
                                 File=self.FullPath,
                                 Line=DefineObj.ModuleType.CurrentLine.LineNo,
                                 ExtraData=DefineObj.ModuleType.CurrentLine.LineString)
                self.LibModuleTypeList.append(ModuleType)
        if DefineObj.GetBaseName() == None:
            Logger.Error("InfParser", FORMAT_INVALID,
                         ST.ERR_INF_PARSER_DEFINE_SECTION_MUST_ITEM_NOT_EXIST % ("BASE_NAME"), File=self.FullPath)
        else:
            self.SetBaseName(DefineObj.GetBaseName().GetValue())
        if DefineObj.GetModuleUniFileName():
            self.UniFileClassObject = UniFileClassObject([PathClass(DefineObj.GetModuleUniFileName())])
        else:
            self.UniFileClassObject = None
        if DefineObj.GetInfVersion() == None:
            Logger.Error("InfParser", FORMAT_INVALID,
                         ST.ERR_INF_PARSER_DEFINE_SECTION_MUST_ITEM_NOT_EXIST % ("INF_VERSION"), File=self.FullPath)
        else:
            self.SetVersion(DefineObj.GetInfVersion().GetValue())
        if DefineObj.GetFileGuid() == None:
            Logger.Error("InfParser", FORMAT_INVALID,
                         ST.ERR_INF_PARSER_DEFINE_SECTION_MUST_ITEM_NOT_EXIST % ("FILE_GUID"), File=self.FullPath)
        else:
            self.SetGuid(DefineObj.GetFileGuid().GetValue())
        if DefineObj.GetVersionString() == None:
            #
            # VERSION_STRING is missing from the [Defines] section, tools must assume that the module's version is 0.
            #
            self.SetVersion('0')
        else:
            #
            # Get version of INF
            #
            if DefineObj.GetVersionString().GetValue() != "":
                #
                # EDK2 inf
                #
                VersionString = DefineObj.GetVersionString().GetValue()
                if len(VersionString) > 0:
                    VersionString = ConvertVersionToDecimal(VersionString)
                    self.SetVersion(VersionString)
            else:
                #
                # EDK1 inf
                #
                Logger.Error("Parser", PARSER_ERROR, ST.ERR_INF_PARSER_NOT_SUPPORT_EDKI_INF, ExtraData=self.FullPath,
                             RaiseError=Logger.IS_RAISE_ERROR)
        #
        # if there is Shadow, Should judge the MODULE_TYPE in 
        # SEC, PEI_CORE and PEIM
        #
        if DefineObj.GetShadow():
            ModuleTypeValue = DefineObj.GetModuleType().GetValue()
            if not (ModuleTypeValue == 'SEC' or ModuleTypeValue == 'PEI_CORE' or ModuleTypeValue == 'PEIM'):
                Logger.Error("InfParser", FORMAT_INVALID, ST.ERR_INF_PARSER_DEFINE_SHADOW_INVALID, File=self.FullPath)

        if DefineObj.GetPcdIsDriver() != None:
            self.SetPcdIsDriver(DefineObj.GetPcdIsDriver().GetValue())
        #
        # LIBRARY_CLASS
        #
        self._GenModuleHeaderLibClass(DefineObj, ArchList)
        #
        # CUSTOM_MAKEFILE
        #
        self.CustomMakefile = DefineObj.GetCustomMakefile()
        #
        # Externs in Defines section
        # Only one define section, so just call once.
        #
        if not HasCalledFlag:
            self._GenModuleHeaderExterns(DefineObj)
            HasCalledFlag = True
        #
        # each module has only one module header
        #
        self.SetSupArchList(ArchList)
        #
        # Get Hob/BootMode/EventList information
        #
        self._GenSpecialComments()
        #
        # put all define statement into user-extension sections
        #     
        DefinesDictNew = GenModuleHeaderUserExt(DefineObj, ArchString)
        if DefinesDictNew:
            UserExtension = CommonObject.UserExtensionObject()
            UserExtension.SetDefinesDict(DefinesDictNew)
            UserExtension.SetIdentifier('DefineModifiers')
            UserExtension.SetUserID('EDK2')
            self.SetUserExtensionList(self.GetUserExtensionList() + [UserExtension])
        #
        # Get all meta-file header information
        # the record is list of items formated: 
        # [LineValue, Arch, StartLine, ID, Third]
        #
        InfHeaderObj = self.Parser.InfHeader
        #
        # Put header information into POM object
        #
        if self.UniFileClassObject:
            Lang = DT.TAB_LANGUAGE_EN_X
        else:
            Lang = DT.TAB_LANGUAGE_EN_US
        if InfHeaderObj.GetAbstract():
            self.SetAbstract((Lang, InfHeaderObj.GetAbstract()))
        if InfHeaderObj.GetDescription():
            self.SetDescription((Lang, InfHeaderObj.GetDescription()))
        if InfHeaderObj.GetCopyright():
            self.SetCopyright(('', InfHeaderObj.GetCopyright()))
        if InfHeaderObj.GetLicense():
            self.SetLicense(('', InfHeaderObj.GetLicense()))
        #
        # Put Binary header information into POM object
        #
        InfBinaryHeaderObj = self.Parser.InfBinaryHeader
        if InfBinaryHeaderObj.GetAbstract():
            self.SetBinaryHeaderAbstract((Lang, InfBinaryHeaderObj.GetAbstract()))
        if InfBinaryHeaderObj.GetDescription():
            self.SetBinaryHeaderDescription((Lang, InfBinaryHeaderObj.GetDescription()))
        if InfBinaryHeaderObj.GetCopyright():
            self.SetBinaryHeaderCopyright(('', InfBinaryHeaderObj.GetCopyright()))
        if InfBinaryHeaderObj.GetLicense():
            self.SetBinaryHeaderLicense(('', InfBinaryHeaderObj.GetLicense()))

    ## GenModuleHeaderLibClass
    #
    #
    def _GenModuleHeaderLibClass(self, DefineObj, ArchList):
        LibraryList = DefineObj.GetLibraryClass()
        for LibraryItem in LibraryList:
            Lib = CommonObject.LibraryClassObject()
            Lib.SetLibraryClass(LibraryItem.GetLibraryName())
            Lib.SetUsage(DT.USAGE_ITEM_PRODUCES)
            SupModuleList = LibraryItem.GetTypes()
            self.LibModuleTypeList += SupModuleList
            Lib.SetSupModuleList(SupModuleList)
            Lib.SetSupArchList(ArchList)
            self.SetLibraryClassList(self.GetLibraryClassList() + [Lib])
            self.SetIsLibrary(True)
            self.SetIsLibraryModList(self.GetIsLibraryModList() + SupModuleList)

    ## GenModuleHeaderExterns
    #
    #
    def _GenModuleHeaderExterns(self, DefineObj):
        EntryPointList = DefineObj.GetEntryPoint()
        for EntryPoint in EntryPointList:
            Image = ExternObject()
            Image.SetEntryPoint(EntryPoint.GetCName())
            #
            # Future enhancement
            #
            self.SetExternList(self.GetExternList() + [Image])
        #
        # UNLOAD_IMAGE
        # 
        UnloadImageList = DefineObj.GetUnloadImages()
        for UnloadImage in UnloadImageList:
            Image = ExternObject()
            #
            # Future enhancement
            #                
            Image.SetUnloadImage(UnloadImage.GetCName())
            self.SetExternList(self.GetExternList() + [Image])
        #
        # CONSTRUCTOR
        #
        ConstructorList = DefineObj.GetConstructor()
        for ConstructorItem in ConstructorList:
            Image = ExternObject()
            #
            # Future enhancement
            #                  
            Image.SetConstructor(ConstructorItem.GetCName())
            self.SetExternList(self.GetExternList() + [Image])
        #
        # DESTRUCTOR
        #
        DestructorList = DefineObj.GetDestructor()
        for DestructorItem in DestructorList:
            Image = ExternObject()
            #
            # Future enhancement
            #                
            Image.SetDestructor(DestructorItem.GetCName())
            self.SetExternList(self.GetExternList() + [Image])

    ## GenModuleHeaderExterns
    # BootMode/HOB/Event
    #
    def _GenSpecialComments(self):
        SpecialCommentsList = self.Parser.InfSpecialCommentSection.GetSpecialComments()
        for Key in SpecialCommentsList:
            if Key == DT.TYPE_HOB_SECTION:
                HobList = []
                for Item in SpecialCommentsList[Key]:
                    Hob = HobObject()
                    Hob.SetHobType(Item.GetHobType())
                    Hob.SetUsage(Item.GetUsage())
                    Hob.SetSupArchList(Item.GetSupArchList())
                    if Item.GetHelpString():
                        HelpTextObj = CommonObject.TextObject()
                        if self.UniFileClassObject:
                            HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                        HelpTextObj.SetString(Item.GetHelpString())
                        Hob.SetHelpTextList([HelpTextObj])
                    HobList.append(Hob)
                self.SetHobList(HobList)
            elif Key == DT.TYPE_EVENT_SECTION:
                EventList = []
                for Item in SpecialCommentsList[Key]:
                    Event = EventObject()
                    Event.SetEventType(Item.GetEventType())
                    Event.SetUsage(Item.GetUsage())
                    if Item.GetHelpString():
                        HelpTextObj = CommonObject.TextObject()
                        if self.UniFileClassObject:
                            HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                        HelpTextObj.SetString(Item.GetHelpString())
                        Event.SetHelpTextList([HelpTextObj])
                    EventList.append(Event)
                self.SetEventList(EventList)
            elif Key == DT.TYPE_BOOTMODE_SECTION:
                BootModeList = []
                for Item in SpecialCommentsList[Key]:
                    BootMode = BootModeObject()
                    BootMode.SetSupportedBootModes(Item.GetSupportedBootModes())
                    BootMode.SetUsage(Item.GetUsage())
                    if Item.GetHelpString():
                        HelpTextObj = CommonObject.TextObject()
                        if self.UniFileClassObject:
                            HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                        HelpTextObj.SetString(Item.GetHelpString())
                        BootMode.SetHelpTextList([HelpTextObj])
                    BootModeList.append(BootMode)
                self.SetBootModeList(BootModeList)

    ## GenBuildOptions
    #
    # Gen BuildOptions of Inf
    # [<Family>:]<ToolFlag>=Flag
    #
    #
    def _GenBuildOptions(self):
        Logger.Debug(2, "Generate %s ..." % DT.TAB_BUILD_OPTIONS)
        #
        # Get all BuildOptions
        #
        BuildOptionsList = self.Parser.InfBuildOptionSection.GetBuildOptions()
        if not GlobalData.gIS_BINARY_INF:
            BuildOptionDict = {}
            for BuildOptionObj in BuildOptionsList:
                ArchList = BuildOptionObj.GetSupArchList()
                ArchList = ConvertArchList(ArchList)
                BuildOptionsContent = BuildOptionObj.GetContent()
                ArchString = ' '.join(ArchList)
                if not BuildOptionsContent:
                    continue
                BuildOptionDict[ArchString] = BuildOptionsContent
            if not BuildOptionDict:
                return
            UserExtension = CommonObject.UserExtensionObject()
            UserExtension.SetBuildOptionDict(BuildOptionDict)
            UserExtension.SetIdentifier('BuildOptionModifiers')
            UserExtension.SetUserID('EDK2')
            self.SetUserExtensionList(self.GetUserExtensionList() + [UserExtension])
        else:
            #
            # Not process this information, will be processed in GenBinaries()
            #
            pass

    ## GenLibraryClasses
    #
    # Get LibraryClass of Inf
    # <LibraryClassKeyWord>|<LibraryInstance>
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenLibraryClasses(self):
        Logger.Debug(2, "Generate %s ..." % DT.TAB_LIBRARY_CLASSES)
        if not GlobalData.gIS_BINARY_INF:
            #
            # Get all LibraryClasses
            #
            LibClassObj = self.Parser.InfLibraryClassSection.LibraryClasses
            Keys = LibClassObj.keys()
            for Key in Keys:
                LibraryClassData = LibClassObj[Key]
                for Item in LibraryClassData:
                    LibraryClass = CommonObject.LibraryClassObject()
                    LibraryClass.SetUsage(DT.USAGE_ITEM_CONSUMES)
                    LibraryClass.SetLibraryClass(Item.GetLibName())
                    LibraryClass.SetRecommendedInstance(None)
                    LibraryClass.SetFeatureFlag(Item.GetFeatureFlagExp())
                    LibraryClass.SetSupArchList(ConvertArchList(Item.GetSupArchList()))
                    LibraryClass.SetSupModuleList(Item.GetSupModuleList())
                    HelpStringObj = Item.GetHelpString()
                    if HelpStringObj != None:
                        CommentString = GetHelpStringByRemoveHashKey(HelpStringObj.HeaderComments +
                                                                     HelpStringObj.TailComments)
                        HelpTextHeaderObj = CommonObject.TextObject()
                        if self.UniFileClassObject:
                            HelpTextHeaderObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                        HelpTextHeaderObj.SetString(CommentString)
                        LibraryClass.SetHelpTextList([HelpTextHeaderObj])
                    self.SetLibraryClassList(self.GetLibraryClassList() + [LibraryClass])

    ## GenPackages
    #
    # Gen Packages of Inf
    #
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenPackages(self, Skip):
        Logger.Debug(2, "Generate %s ..." % DT.TAB_PACKAGES)
        #
        # Get all Packages
        #
        PackageObj = self.Parser.InfPackageSection.Packages
        #
        # Go through each arch
        #
        for PackageItemObj in PackageObj:
            #
            # Need package information for dependency check usage
            #
            PackageDependency = PackageDependencyObject()
            PackageDependency.SetPackageFilePath(NormPath(PackageItemObj.GetPackageName()))
            PackageDependency.SetSupArchList(ConvertArchList(PackageItemObj.GetSupArchList()))
            PackageDependency.SetFeatureFlag(PackageItemObj.GetFeatureFlagExp())

            PkgInfo = GetPkgInfoFromDec(os.path.normpath(os.path.join(self.WorkSpace,
                                                                      NormPath(PackageItemObj.GetPackageName()))))
            if PkgInfo[1] and PkgInfo[2]:
                PackageDependency.SetGuid(PkgInfo[1])
                PackageDependency.SetVersion(PkgInfo[2])
            elif Skip:
                continue
            else:
                Logger.Error("\nUPT", PARSER_ERROR,
                             ST.ERR_INF_GET_PKG_DEPENDENCY_FAIL % PackageItemObj.GetPackageName(), File=self.FullPath)

            PackageDependencyList = self.GetPackageDependencyList()
            PackageDependencyList.append(PackageDependency)
            self.SetPackageDependencyList(PackageDependencyList)

    ## GenPcds
    #
    # Gen Pcds of Inf
    # <TokenSpaceGuidCName>.<PcdCName>[|<Value> [|<FFE>]]
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenPcds(self):
        if not GlobalData.gIS_BINARY_INF:
            Logger.Debug(2, "Generate %s ..." % DT.TAB_PCDS)
            #
            # Get all Pcds
            #
            PcdObj = self.Parser.InfPcdSection.Pcds
            KeysList = PcdObj.keys()
            #
            # Go through each arch
            #
            for (PcdType, PcdKey) in KeysList:
                PcdData = PcdObj[PcdType, PcdKey]
                for PcdItemObj in PcdData:
                    CommentList = PcdItemObj.GetHelpStringList()
                    if CommentList:
                        for CommentItem in CommentList:
                            Pcd = CommonObject.PcdObject()
                            Pcd.SetCName(PcdItemObj.GetCName())
                            Pcd.SetTokenSpaceGuidCName(PcdItemObj.GetTokenSpaceGuidCName())
                            Pcd.SetDefaultValue(PcdItemObj.GetDefaultValue())
                            Pcd.SetItemType(PcdType)
                            Pcd.SetValidUsage(CommentItem.GetUsageItem())
                            Pcd.SetFeatureFlag(PcdItemObj.GetFeatureFlagExp())
                            Pcd.SetSupArchList(ConvertArchList(PcdItemObj.GetSupportArchList()))
                            HelpTextObj = CommonObject.TextObject()
                            if self.UniFileClassObject:
                                HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                            HelpTextObj.SetString(CommentItem.GetHelpStringItem())
                            Pcd.SetHelpTextList([HelpTextObj])
                            PcdList = self.GetPcdList()
                            PcdList.append(Pcd)
                self.SetPcdList(PcdList)

    ## GenSources
    #
    # Gen Sources of Inf
    # <Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenSources(self):
        Logger.Debug(2, "Generate %s ..." % DT.TAB_SOURCES)

        #
        # Get all SourceFiles
        #
        SourceObj = self.Parser.InfSourcesSection.Sources
        DataList = SourceObj.keys()
        #
        # Go through each arch
        #
        SourceList = []
        for Key in DataList:
            SourceData = SourceObj[Key]
            for Item in SourceData:
                SourceFile = Item.GetSourceFileName()
                Family = Item.GetFamily()
                FeatureFlag = Item.GetFeatureFlagExp()
                SupArchList = ConvertArchList(Item.GetSupArchList())
                SupArchList.sort()
                Source = SourceFileObject()
                Source.SetSourceFile(SourceFile)
                Source.SetFamily(Family)
                Source.SetFeatureFlag(FeatureFlag)
                Source.SetSupArchList(SupArchList)
                SourceList.append(Source)

        self.SetSourceFileList(self.GetSourceFileList() + SourceList)


    ## GenUserExtensions
    #
    # Gen UserExtensions of Inf
    #
    def _GenUserExtensions(self):
        #
        # UserExtensions
        #
        UserExtensionObj = self.Parser.InfUserExtensionSection.UserExtension
        Keys = UserExtensionObj.keys()

        for Key in Keys:
            UserExtensionData = UserExtensionObj[Key]
            for UserExtensionDataObj in UserExtensionData:
                UserExtension = CommonObject.UserExtensionObject()
                UserId = UserExtensionDataObj.GetUserId()
                if UserId.startswith('"') and UserId.endswith('"'):
                    UserId = UserId[1:-1]                
                UserExtension.SetUserID(UserId)
                Identifier = UserExtensionDataObj.GetIdString()
                if Identifier.startswith('"') and Identifier.endswith('"'):
                    Identifier = Identifier[1:-1]
                #
                # Generate miscellaneous files on INF file
                #
                if UserId == 'TianoCore' and Identifier == 'ExtraFiles':
                    self._GenMiscFiles(UserExtensionDataObj.GetContent())
                UserExtension.SetIdentifier(Identifier)
                UserExtension.SetStatement(UserExtensionDataObj.GetContent())
                UserExtension.SetSupArchList(ConvertArchList(UserExtensionDataObj.GetSupArchList()))
                self.SetUserExtensionList(self.GetUserExtensionList() + [UserExtension])
                
        #
        #  Gen UserExtensions of TianoCore."BinaryHeader"
        #
        
        #Get Binary header from INF file
        BinaryAbstractList = self.BinaryHeaderAbstractList
        BinaryDescriptionList = self.BinaryHeaderDescriptionList
        BinaryCopyrightList = self.BinaryHeaderCopyrightList
        BinaryLicenseList = self.BinaryHeaderLicenseList
        #Get Binary header from UNI file
        # Initialize UniStrDict, the top keys are language codes
        UniStrDict = {}
        if self.UniFileClassObject:
            UniStrDict = self.UniFileClassObject.OrderedStringList
            for Lang in UniStrDict:
                for StringDefClassObject in UniStrDict[Lang]:
                    Lang = GetLanguageCode1766(Lang)
                    if StringDefClassObject.StringName == DT.TAB_INF_BINARY_ABSTRACT:
                        BinaryAbstractList.append((Lang, ConvertSpecialUnicodes(StringDefClassObject.StringValue)))
                    if StringDefClassObject.StringName == DT.TAB_INF_BINARY_DESCRIPTION:
                        BinaryDescriptionList.append((Lang, ConvertSpecialUnicodes(StringDefClassObject.StringValue)))
        if BinaryAbstractList or BinaryDescriptionList or BinaryCopyrightList or BinaryLicenseList:
            BinaryUserExtension = CommonObject.UserExtensionObject()
            BinaryUserExtension.SetBinaryAbstract(BinaryAbstractList)
            BinaryUserExtension.SetBinaryDescription(BinaryDescriptionList)
            BinaryUserExtension.SetBinaryCopyright(BinaryCopyrightList)
            BinaryUserExtension.SetBinaryLicense(BinaryLicenseList)
            BinaryUserExtension.SetIdentifier(DT.TAB_BINARY_HEADER_IDENTIFIER)
            BinaryUserExtension.SetUserID(DT.TAB_BINARY_HEADER_USERID)
            self.SetUserExtensionList(self.GetUserExtensionList() + [BinaryUserExtension])
            
    def _GenDepexesList(self, SmmDepexList, DxeDepexList, PeiDepexList):
        if SmmDepexList:
            self.SetSmmDepex(SmmDepexList)
        if DxeDepexList:
            self.SetDxeDepex(DxeDepexList)
        if PeiDepexList:
            self.SetPeiDepex(PeiDepexList)

    ## GenDepexes
    #
    # Gen Depex of Inf
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenDepexes(self):
        Logger.Debug(2, "Generate %s ..." % DT.TAB_DEPEX)

        PEI_LIST = [DT.SUP_MODULE_PEIM]
        SMM_LIST = [DT.SUP_MODULE_DXE_SMM_DRIVER]
        DXE_LIST = [DT.SUP_MODULE_DXE_DRIVER, DT.SUP_MODULE_DXE_SAL_DRIVER,
                    DT.SUP_MODULE_DXE_RUNTIME_DRIVER]

        IsLibraryClass = self.GetIsLibrary()
        #
        # Get all Depexes
        #
        DepexData = self.Parser.InfDepexSection.GetDepex()
        SmmDepexList = []
        DxeDepexList = []
        PeiDepexList = []
        for Depex in DepexData:
            ModuleType = Depex.GetModuleType()
            ModuleTypeList = []
            if IsLibraryClass:
                if self.GetModuleType() == 'BASE' and not ModuleType:
                    Logger.Error("\nMkPkg", PARSER_ERROR,
                                 ST.ERR_INF_PARSER_DEPEX_SECTION_INVALID_FOR_BASE_LIBRARY_CLASS,
                                 self.GetFullPath(), RaiseError=True)
                if self.GetModuleType() != 'BASE' and not self.GetIsLibraryModList():
                    Logger.Error("\nMkPkg", PARSER_ERROR, ST.ERR_INF_PARSER_DEPEX_SECTION_INVALID_FOR_LIBRARY_CLASS,
                                 self.GetFullPath(), RaiseError=True)
                if self.GetModuleType() != 'BASE' and ModuleType and ModuleType not in self.GetIsLibraryModList():
                    Logger.Error("\nMkPkg", PARSER_ERROR, ST.ERR_INF_PARSER_DEPEX_SECTION_NOT_DETERMINED,
                                 self.GetFullPath(), RaiseError=True)
                if ModuleType:
                    ModuleTypeList = [ModuleType]
                else:
                    for ModuleTypeInList in self.GetIsLibraryModList():
                        if ModuleTypeInList in DT.VALID_DEPEX_MODULE_TYPE_LIST:
                            ModuleTypeList.append(ModuleTypeInList)
                if not ModuleTypeList:
                    Logger.Error("\nMkPkg", PARSER_ERROR, ST.ERR_INF_PARSER_DEPEX_SECTION_NOT_DETERMINED,
                                 self.GetFullPath(), RaiseError=True)
            else:
                if not ModuleType:
                    ModuleType = self.ModuleType
                if ModuleType not in DT.VALID_DEPEX_MODULE_TYPE_LIST:
                    Logger.Error("\nMkPkg", PARSER_ERROR,
                                 ST.ERR_INF_PARSER_DEPEX_SECTION_MODULE_TYPE_ERROR % (ModuleType),
                                 self.GetFullPath(), RaiseError=True)
                if ModuleType != self.ModuleType:
                    Logger.Error("\nMkPkg", PARSER_ERROR, ST.ERR_INF_PARSER_DEPEX_SECTION_NOT_DETERMINED,
                                 self.GetFullPath(), RaiseError=True)
                ModuleTypeList = [ModuleType]
            for ModuleType in ModuleTypeList:
                DepexIns = DepexObject()
                DepexIns.SetDepex(Depex.GetDepexContent())
                if IsLibraryClass:
                    DepexIns.SetModuleType(ModuleType)
                else:
                    if Depex.GetModuleType():
                        DepexIns.SetModuleType(Depex.GetModuleType())
                DepexIns.SetSupArchList(ConvertArchList([Depex.GetSupArch()]))
                DepexIns.SetFeatureFlag(Depex.GetFeatureFlagExp())
                if Depex.HelpString:
                    HelpIns = CommonObject.TextObject()
                    if self.UniFileClassObject:
                        HelpIns.SetLang(DT.TAB_LANGUAGE_EN_X)
                    HelpIns.SetString(GetHelpStringByRemoveHashKey(Depex.HelpString))
                    DepexIns.SetHelpText(HelpIns)

                if ModuleType in SMM_LIST:
                    SmmDepexList.append(DepexIns)
                if ModuleType in DXE_LIST:
                    DxeDepexList.append(DepexIns)
                if ModuleType in PEI_LIST:
                    PeiDepexList.append(DepexIns)
                if ModuleType == DT.SUP_MODULE_UEFI_DRIVER:
                    if IsLibraryClass:
                        DxeDepexList.append(DepexIns)
                    else:
                        Logger.Error("\nMkPkg", PARSER_ERROR, ST.ERR_INF_PARSER_DEPEX_SECTION_INVALID_FOR_DRIVER,
                                     self.GetFullPath(), RaiseError=True)

            #End of for ModuleType in ModuleTypeList
            self._GenDepexesList(SmmDepexList, DxeDepexList, PeiDepexList)
        #End of for Depex in DepexData

    ## GenBinaries
    #
    # Gen Binary of Inf, must be called after Pcd/Library is generated
    # <FileType>|<Filename>|<Target>[|<TokenSpaceGuidCName>.<PcdCName>]
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenBinaries(self):
        Logger.Debug(2, "Generate %s ..." % DT.TAB_BINARIES)
        BinariesDict = {}

        #
        # Get all Binary data
        #
        BinaryObj = self.Parser.InfBinariesSection.GetBinary()

        BinaryData = BinaryObj.keys()

        #
        # If the INF file does not contain a [Sources] section, and the INF file does contain a [Binaries] section, 
        # then the ModuleSurfaceArea.BinaryModule attribute must be set to true. Otherwise, do not use the attribute
        #
        if BinaryObj and not self.Parser.InfSourcesSection.GetSources():
            self.BinaryModule = True
        else:
            self.BinaryModule = False

        BinaryFileObjectList = []
        AsBuildLibraryClassList = []
        AsBuildBuildOptionList = []
        AsBuildIns = AsBuiltObject()
        #
        # Library AsBuild Info
        #
        for LibItem in self.Parser.InfLibraryClassSection.GetLibraryClasses():
            AsBuildLibIns = AsBuildLibraryClassObject()
            AsBuildLibIns.SetLibGuid(LibItem.GetFileGuid())
            AsBuildLibIns.SetLibVersion(LibItem.GetVersion())
            AsBuildLibIns.SetSupArchList(LibItem.GetSupArchList())
            AsBuildLibraryClassList.append(AsBuildLibIns)
        AsBuildIns.SetLibraryInstancesList(AsBuildLibraryClassList)

        #
        # BuildOption AsBuild Info
        #            
        for BuildOptionItem in self.Parser.InfBuildOptionSection.GetBuildOptions():
            AsBuildBuildOptionList.append(BuildOptionItem)
        AsBuildIns.SetBuildFlagsList(AsBuildBuildOptionList)

        #
        # PatchPcd and PcdEx
        #
        AsBuildIns = self._GenAsBuiltPcds(self.Parser.InfPcdSection.GetPcds(), AsBuildIns)

        #
        # Parse the DEC file that contains the GUID value of the GUID CName which is used by
        # SUBTYPE_GUID type binary file in the Binaries section in the INF file
        #
        DecObjList = []
        if not self.PackagePath:
            WorkSpace = os.path.normpath(self.WorkSpace)
            TempPath = ModulePath = os.path.normpath(self.ModulePath)
            while ModulePath:
                TempPath = ModulePath
                ModulePath = os.path.dirname(ModulePath)
            PackageName = TempPath
            DecFilePath = os.path.normpath(os.path.join(WorkSpace, PackageName))  
            if DecFilePath:
                for File in os.listdir(DecFilePath):
                    if File.upper().endswith('.DEC'):
                        DecFileFullPath = os.path.normpath(os.path.join(DecFilePath, File))
                        DecObjList.append(DecPomAlignment(DecFileFullPath, self.WorkSpace))
                        
        
        BinariesDict, AsBuildIns, BinaryFileObjectList = GenBinaryData(BinaryData, BinaryObj,
                                                                       BinariesDict,
                                                                       AsBuildIns,
                                                                       BinaryFileObjectList,
                                                                       self.GetSupArchList(),
                                                                       self.BinaryModule,
                                                                       DecObjList)
                
        BinariesDict2 = {}
        for Key in BinariesDict:
            ValueList = BinariesDict[Key]
            if len(ValueList) > 1:
                BinariesDict2[Key] = ValueList
            else:
                #
                # if there is no TagName, ToolCode, HelpStr, 
                # then do not need to put them into userextension
                #
                (Target, Family, TagName, HelpStr) = ValueList[0]
                if not (Target or Family or TagName or HelpStr):
                    continue
                else:
                    BinariesDict2[Key] = ValueList

        self.SetBinaryFileList(self.GetBinaryFileList() + BinaryFileObjectList)

        if BinariesDict2:
            UserExtension = CommonObject.UserExtensionObject()
            UserExtension.SetBinariesDict(BinariesDict2)
            UserExtension.SetIdentifier('BinaryFileModifiers')
            UserExtension.SetUserID('EDK2')
            self.SetUserExtensionList(self.GetUserExtensionList() + [UserExtension])

    ## GenAsBuiltPcds
    #
    #
    def _GenAsBuiltPcds(self, PcdList, AsBuildIns):
        AsBuildPatchPcdList = []
        AsBuildPcdExList = []
        #
        # Pcd AsBuild Info
        #
        for PcdItem in PcdList:
            if PcdItem[0].upper() == DT.TAB_INF_PATCH_PCD.upper():
                PcdItemObj = PcdItem[1]
                Pcd = CommonObject.PcdObject()
                Pcd.SetCName(PcdItemObj.GetCName())
                Pcd.SetTokenSpaceGuidCName(PcdItemObj.GetTokenSpaceGuidCName())
                if PcdItemObj.GetTokenSpaceGuidValue() == '' and self.BinaryModule:
                    Logger.Error("\nMkPkg",
                                 PARSER_ERROR,
                                 ST.ERR_ASBUILD_PCD_TOKENSPACE_GUID_VALUE_MISS % \
                                 (PcdItemObj.GetTokenSpaceGuidCName()),
                                 self.GetFullPath(), RaiseError=True)
                else:
                    Pcd.SetTokenSpaceGuidValue(PcdItemObj.GetTokenSpaceGuidValue())
                if (PcdItemObj.GetToken() == '' or PcdItemObj.GetDatumType() == '') and self.BinaryModule:
                    Logger.Error("\nMkPkg",
                                 PARSER_ERROR,
                                 ST.ERR_ASBUILD_PCD_DECLARITION_MISS % \
                                 (PcdItemObj.GetTokenSpaceGuidCName() + '.' + PcdItemObj.GetCName()),
                                 self.GetFullPath(), RaiseError=True)
                Pcd.SetToken(PcdItemObj.GetToken())
                Pcd.SetDatumType(PcdItemObj.GetDatumType())
                Pcd.SetMaxDatumSize(PcdItemObj.GetMaxDatumSize())
                Pcd.SetDefaultValue(PcdItemObj.GetDefaultValue())
                Pcd.SetOffset(PcdItemObj.GetOffset())
                Pcd.SetItemType(PcdItem[0])
                Pcd.SetFeatureFlag(PcdItemObj.GetFeatureFlagExp())
                Pcd.SetSupArchList(ConvertArchList(PcdItemObj.GetSupportArchList()))
                Pcd.SetValidUsage(PcdItemObj.GetValidUsage())
                for CommentItem in PcdItemObj.GetHelpStringList():
                    HelpTextObj = CommonObject.TextObject()
                    if self.UniFileClassObject:
                        HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                    HelpTextObj.SetString(CommentItem.GetHelpStringItem())
                    Pcd.SetHelpTextList(Pcd.GetHelpTextList() + [HelpTextObj])
                AsBuildPatchPcdList.append(Pcd)
            elif PcdItem[0].upper() == DT.TAB_INF_PCD_EX.upper():
                PcdItemObj = PcdItem[1]
                Pcd = CommonObject.PcdObject()
                Pcd.SetTokenSpaceGuidValue(PcdItemObj.GetTokenSpaceGuidValue())
                Pcd.SetToken(PcdItemObj.GetToken())
                Pcd.SetDatumType(PcdItemObj.GetDatumType())
                Pcd.SetMaxDatumSize(PcdItemObj.GetMaxDatumSize())
                Pcd.SetDefaultValue(PcdItemObj.GetDefaultValue())
                Pcd.SetItemType(PcdItem[0])
                Pcd.SetFeatureFlag(PcdItemObj.GetFeatureFlagExp())
                Pcd.SetSupArchList(ConvertArchList(PcdItemObj.GetSupportArchList()))
                Pcd.SetValidUsage(PcdItemObj.GetValidUsage())
                for CommentItem in PcdItemObj.GetHelpStringList():
                    HelpTextObj = CommonObject.TextObject()
                    if self.UniFileClassObject:
                        HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                    HelpTextObj.SetString(CommentItem.GetHelpStringItem())
                    Pcd.SetHelpTextList(Pcd.GetHelpTextList() + [HelpTextObj])
                AsBuildPcdExList.append(Pcd)
        AsBuildIns.SetPatchPcdList(AsBuildPatchPcdList)
        AsBuildIns.SetPcdExList(AsBuildPcdExList)

        return AsBuildIns

    ## GenGuidProtocolPpis
    #
    # Gen Guids/Protocol/Ppis of INF
    # <CName>=<GuidValue>
    #
    def _GenGuidProtocolPpis(self, Type):
        Logger.Debug(2, "Generate %s ..." % Type)
        #
        # Get all Guid/Protocol/Ppis data
        #
        GuidObj = self.Parser.InfGuidSection.GetGuid()
        ProtocolObj = self.Parser.InfProtocolSection.GetProtocol()
        PpisObj = self.Parser.InfPpiSection.GetPpi()

        GuidProtocolPpiList = []

        if Type == DT.TAB_GUIDS:
            GuidData = GuidObj.keys()
            for Item in GuidData:
                CommentList = Item.GetCommentList()
                #
                # Depend on CommentList content
                # generate muti-guid-obj
                #
                if CommentList:
                    for GuidComentItem in CommentList:
                        ListObject = CommonObject.GuidObject()
                        ListObject.SetGuidTypeList([GuidComentItem.GetGuidTypeItem()])
                        ListObject.SetVariableName(GuidComentItem.GetVariableNameItem())
                        ListObject.SetUsage(GuidComentItem.GetUsageItem())
                        ListObject.SetName(Item.GetName())
                        ListObject.SetCName(Item.GetName())
                        ListObject.SetSupArchList(ConvertArchList(Item.GetSupArchList()))
                        ListObject.SetFeatureFlag(Item.GetFeatureFlagExp())
                        HelpString = GuidComentItem.GetHelpStringItem()
                        if HelpString.strip():
                            HelpTxtTailObj = CommonObject.TextObject()
                            if self.UniFileClassObject:
                                HelpTxtTailObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                            HelpTxtTailObj.SetString(HelpString)
                            ListObject.SetHelpTextList([HelpTxtTailObj])
                            
                        GuidProtocolPpiList.append(ListObject)
        elif Type == DT.TAB_PROTOCOLS:
            ProtocolData = ProtocolObj.keys()
            for Item in ProtocolData:
                CommentList = Item.GetCommentList()
                for CommentItem in CommentList:
                    ListObject = CommonObject.ProtocolObject()
                    ListObject.SetCName(Item.GetName())
                    ListObject.SetSupArchList(ConvertArchList(Item.GetSupArchList()))
                    ListObject.SetFeatureFlag(Item.GetFeatureFlagExp())
                    ListObject.SetNotify(CommentItem.GetNotify())
                    ListObject.SetUsage(CommentItem.GetUsageItem())
                    HelpString = CommentItem.GetHelpStringItem()
                    if HelpString.strip():
                        HelpTxtObj = CommonObject.TextObject()
                        if self.UniFileClassObject:
                            HelpTxtObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                        HelpTxtObj.SetString(HelpString)
                        ListObject.SetHelpTextList([HelpTxtObj])
                    GuidProtocolPpiList.append(ListObject)
        elif Type == DT.TAB_PPIS:
            PpiData = PpisObj.keys()
            for Item in PpiData:
                CommentList = Item.GetCommentList()
                for CommentItem in CommentList:
                    ListObject = CommonObject.PpiObject()
                    ListObject.SetCName(Item.GetName())
                    ListObject.SetSupArchList(ConvertArchList(Item.GetSupArchList()))
                    ListObject.SetFeatureFlag(Item.GetFeatureFlagExp())
                    ListObject.SetNotify(CommentItem.GetNotify())
                    ListObject.SetUsage(CommentItem.GetUsage())
                    HelpString = CommentItem.GetHelpStringItem()
                    if HelpString.strip():
                        HelpTextObj = CommonObject.TextObject()
                        if self.UniFileClassObject:
                            HelpTextObj.SetLang(DT.TAB_LANGUAGE_EN_X)
                        HelpTextObj.SetString(HelpString)
                        ListObject.SetHelpTextList([HelpTextObj])
                    GuidProtocolPpiList.append(ListObject)

        if Type == DT.TAB_GUIDS:
            self.SetGuidList(self.GetGuidList() + GuidProtocolPpiList)
        elif Type == DT.TAB_PROTOCOLS:
            self.SetProtocolList(self.GetProtocolList() + GuidProtocolPpiList)
        elif Type == DT.TAB_PPIS:
            self.SetPpiList(self.GetPpiList() + GuidProtocolPpiList)

    ## GenMiscFiles
    #
    # Gen MiscellaneousFiles of Inf
    #
    # @param ContainerFile: The Inf file full path
    #
    def _GenMiscFiles(self, Content):
        MiscFileObj = CommonObject.MiscFileObject()
        for Line in Content.splitlines():
            FileName = ''
            if '#' in Line:
                FileName = Line[:Line.find('#')]
            else:
                FileName = Line
            if FileName:
                if IsValidPath(FileName, GlobalData.gINF_MODULE_DIR):
                    FileObj = CommonObject.FileObject()
                    FileObj.SetURI(FileName)
                    MiscFileObj.SetFileList(MiscFileObj.GetFileList()+[FileObj])
                else:
                    Logger.Error("InfParser", 
                                 FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(Line),
                                 File=GlobalData.gINF_MODULE_NAME,
                                 ExtraData=Line)   
        self.SetMiscFileList(self.GetMiscFileList()+[MiscFileObj]) 
        
