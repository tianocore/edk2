## @file GenInfFile.py
#
# This file contained the logical of transfer package object to INF files.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
'''
GenInf
'''
import os
import stat
import codecs
from hashlib import md5
from Core.FileHook import __FileHookOpen__
from Library.StringUtils import GetSplitValueList
from Library.Parsing import GenSection
from Library.Parsing import GetWorkspacePackage
from Library.Parsing import ConvertArchForInstall
from Library.Misc import SaveFileOnChange
from Library.Misc import IsAllModuleList
from Library.Misc import Sdict
from Library.Misc import ConvertPath
from Library.Misc import ConvertSpec
from Library.Misc import GetRelativePath
from Library.Misc import GetLocalValue
from Library.CommentGenerating import GenHeaderCommentSection
from Library.CommentGenerating import GenGenericCommentF
from Library.CommentGenerating import _GetHelpStr
from Library import GlobalData
from Logger import StringTable as ST
from Logger import ToolError
import Logger.Log as Logger
from Library import DataType as DT
from GenMetaFile import GenMetaFileMisc
from Library.UniClassObject import FormatUniEntry
from Library.StringUtils import GetUniFileName


## Transfer Module Object to Inf files
#
# Transfer all contents of a standard Module Object to an Inf file
# @param ModuleObject: A Module Object
#
def ModuleToInf(ModuleObject, PackageObject=None, DistHeader=None):
    if not GlobalData.gWSPKG_LIST:
        GlobalData.gWSPKG_LIST = GetWorkspacePackage()
    #
    # Init global information for the file
    #
    ContainerFile = ModuleObject.GetFullPath()

    Content = ''
    #
    # Generate file header, If any Abstract, Description, Copyright or License XML elements are missing,
    # should 1) use the Abstract, Description, Copyright or License from the PackageSurfaceArea.Header elements
    # that the module belongs to, or 2) if this is a stand-alone module that is not included in a PackageSurfaceArea,
    # use the abstract, description, copyright or license from the DistributionPackage.Header elements.
    #
    ModuleAbstract = GetLocalValue(ModuleObject.GetAbstract())
    if not ModuleAbstract and PackageObject:
        ModuleAbstract = GetLocalValue(PackageObject.GetAbstract())
    if not ModuleAbstract and DistHeader:
        ModuleAbstract = GetLocalValue(DistHeader.GetAbstract())
    ModuleDescription = GetLocalValue(ModuleObject.GetDescription())
    if not ModuleDescription and PackageObject:
        ModuleDescription = GetLocalValue(PackageObject.GetDescription())
    if not ModuleDescription and DistHeader:
        ModuleDescription = GetLocalValue(DistHeader.GetDescription())
    ModuleCopyright = ''
    for (Lang, Copyright) in ModuleObject.GetCopyright():
        if Lang:
            pass
        ModuleCopyright = Copyright
    if not ModuleCopyright and PackageObject:
        for (Lang, Copyright) in PackageObject.GetCopyright():
            if Lang:
                pass
            ModuleCopyright = Copyright
    if not ModuleCopyright and DistHeader:
        for (Lang, Copyright) in DistHeader.GetCopyright():
            if Lang:
                pass
            ModuleCopyright = Copyright
    ModuleLicense = ''
    for (Lang, License) in ModuleObject.GetLicense():
        if Lang:
            pass
        ModuleLicense = License
    if not ModuleLicense and PackageObject:
        for (Lang, License) in PackageObject.GetLicense():
            if Lang:
                pass
            ModuleLicense = License
    if not ModuleLicense and DistHeader:
        for (Lang, License) in DistHeader.GetLicense():
            if Lang:
                pass
            ModuleLicense = License

    #
    # Generate header comment section of INF file
    #
    Content += GenHeaderCommentSection(ModuleAbstract,
                                       ModuleDescription,
                                       ModuleCopyright,
                                       ModuleLicense).replace('\r\n', '\n')

    #
    # Generate Binary Header
    #
    for UserExtension in ModuleObject.GetUserExtensionList():
        if UserExtension.GetUserID() == DT.TAB_BINARY_HEADER_USERID \
        and UserExtension.GetIdentifier() == DT.TAB_BINARY_HEADER_IDENTIFIER:
            ModuleBinaryAbstract = GetLocalValue(UserExtension.GetBinaryAbstract())
            ModuleBinaryDescription = GetLocalValue(UserExtension.GetBinaryDescription())
            ModuleBinaryCopyright = ''
            ModuleBinaryLicense = ''
            for (Lang, Copyright) in UserExtension.GetBinaryCopyright():
                ModuleBinaryCopyright = Copyright
            for (Lang, License) in UserExtension.GetBinaryLicense():
                ModuleBinaryLicense = License
            if ModuleBinaryAbstract and ModuleBinaryDescription and \
            ModuleBinaryCopyright and ModuleBinaryLicense:
                Content += GenHeaderCommentSection(ModuleBinaryAbstract,
                                           ModuleBinaryDescription,
                                           ModuleBinaryCopyright,
                                           ModuleBinaryLicense,
                                           True)

    #
    # Generate MODULE_UNI_FILE for module
    #
    FileHeader = GenHeaderCommentSection(ModuleAbstract, ModuleDescription, ModuleCopyright, ModuleLicense, False, \
                                         DT.TAB_COMMENT_EDK1_SPLIT)
    ModuleUniFile = GenModuleUNIEncodeFile(ModuleObject, FileHeader)
    if ModuleUniFile:
        ModuleObject.SetModuleUniFile(os.path.basename(ModuleUniFile))

    #
    # Judge whether the INF file is an AsBuild INF.
    #
    if ModuleObject.BinaryModule:
        GlobalData.gIS_BINARY_INF = True
    else:
        GlobalData.gIS_BINARY_INF = False
    #
    # for each section, maintain a dict, sorted arch will be its key,
    # statement list will be its data
    # { 'Arch1 Arch2 Arch3': [statement1, statement2],
    #   'Arch1' : [statement1, statement3]
    #  }
    #
    # Gen section contents
    #
    Content += GenDefines(ModuleObject)
    Content += GenBuildOptions(ModuleObject)
    Content += GenLibraryClasses(ModuleObject)
    Content += GenPackages(ModuleObject)
    Content += GenPcdSections(ModuleObject)
    Content += GenSources(ModuleObject)
    Content += GenProtocolPPiSections(ModuleObject.GetProtocolList(), True)
    Content += GenProtocolPPiSections(ModuleObject.GetPpiList(), False)
    Content += GenGuidSections(ModuleObject.GetGuidList())
    Content += GenBinaries(ModuleObject)
    Content += GenDepex(ModuleObject)
    __UserExtensionsContent = GenUserExtensions(ModuleObject)
    Content += __UserExtensionsContent
    if ModuleObject.GetEventList() or ModuleObject.GetBootModeList() or ModuleObject.GetHobList():
        Content += '\n'
    #
    # generate [Event], [BootMode], [Hob] section
    #
    Content += GenSpecialSections(ModuleObject.GetEventList(), 'Event', __UserExtensionsContent)
    Content += GenSpecialSections(ModuleObject.GetBootModeList(), 'BootMode', __UserExtensionsContent)
    Content += GenSpecialSections(ModuleObject.GetHobList(), 'Hob', __UserExtensionsContent)
    SaveFileOnChange(ContainerFile, Content, False)
    if DistHeader.ReadOnly:
        os.chmod(ContainerFile, stat.S_IRUSR|stat.S_IRGRP|stat.S_IROTH)
    else:
        os.chmod(ContainerFile, stat.S_IRUSR|stat.S_IRGRP|stat.S_IROTH|stat.S_IWUSR|stat.S_IWGRP|stat.S_IWOTH)
    return ContainerFile

## GenModuleUNIEncodeFile
# GenModuleUNIEncodeFile, default is a UCS-2LE encode file
#
def GenModuleUNIEncodeFile(ModuleObject, UniFileHeader='', Encoding=DT.TAB_ENCODING_UTF16LE):
    GenUNIFlag = False
    OnlyLANGUAGE_EN_X = True
    BinaryAbstract = []
    BinaryDescription = []
    #
    # If more than one language code is used for any element that would be present in the MODULE_UNI_FILE,
    # then the MODULE_UNI_FILE must be created.
    #
    for (Key, Value) in ModuleObject.GetAbstract() + ModuleObject.GetDescription():
        if Key == DT.TAB_LANGUAGE_EN_X:
            GenUNIFlag = True
        else:
            OnlyLANGUAGE_EN_X = False

    for UserExtension in ModuleObject.GetUserExtensionList():
        if UserExtension.GetUserID() == DT.TAB_BINARY_HEADER_USERID \
        and UserExtension.GetIdentifier() == DT.TAB_BINARY_HEADER_IDENTIFIER:
            for (Key, Value) in UserExtension.GetBinaryAbstract():
                if Key == DT.TAB_LANGUAGE_EN_X:
                    GenUNIFlag = True
                else:
                    OnlyLANGUAGE_EN_X = False
                BinaryAbstract.append((Key, Value))
            for (Key, Value) in UserExtension.GetBinaryDescription():
                if Key == DT.TAB_LANGUAGE_EN_X:
                    GenUNIFlag = True
                else:
                    OnlyLANGUAGE_EN_X = False
                BinaryDescription.append((Key, Value))


    if not GenUNIFlag:
        return
    elif OnlyLANGUAGE_EN_X:
        return
    else:
        ModuleObject.UNIFlag = True
    ContainerFile = GetUniFileName(os.path.dirname(ModuleObject.GetFullPath()), ModuleObject.GetBaseName())

    if not os.path.exists(os.path.dirname(ModuleObject.GetFullPath())):
        os.makedirs(os.path.dirname(ModuleObject.GetFullPath()))

    Content = UniFileHeader + '\r\n'
    Content += '\r\n'

    Content += FormatUniEntry('#string ' + DT.TAB_INF_ABSTRACT, ModuleObject.GetAbstract(), ContainerFile) + '\r\n'

    Content += FormatUniEntry('#string ' + DT.TAB_INF_DESCRIPTION, ModuleObject.GetDescription(), ContainerFile) \
            + '\r\n'

    BinaryAbstractString = FormatUniEntry('#string ' + DT.TAB_INF_BINARY_ABSTRACT, BinaryAbstract, ContainerFile)
    if BinaryAbstractString:
        Content += BinaryAbstractString + '\r\n'

    BinaryDescriptionString = FormatUniEntry('#string ' + DT.TAB_INF_BINARY_DESCRIPTION, BinaryDescription, \
                                             ContainerFile)
    if BinaryDescriptionString:
        Content += BinaryDescriptionString + '\r\n'

    if not os.path.exists(ContainerFile):
        File = codecs.open(ContainerFile, 'wb', Encoding)
        File.write(u'\uFEFF' + Content)
        File.stream.close()
    Md5Signature = md5(__FileHookOpen__(str(ContainerFile), 'rb').read())
    Md5Sum = Md5Signature.hexdigest()
    if (ContainerFile, Md5Sum) not in ModuleObject.FileList:
        ModuleObject.FileList.append((ContainerFile, Md5Sum))

    return ContainerFile
def GenDefines(ModuleObject):
    #
    # generate [Defines] section
    #
    LeftOffset = 31
    Content = ''
    NewSectionDict = {}

    for UserExtension in ModuleObject.GetUserExtensionList():
        DefinesDict = UserExtension.GetDefinesDict()
        if not DefinesDict:
            continue
        for Statement in DefinesDict:
            if len(Statement.split(DT.TAB_EQUAL_SPLIT)) > 1:
                Statement = (u'%s ' % Statement.split(DT.TAB_EQUAL_SPLIT, 1)[0]).ljust(LeftOffset) \
                             + u'= %s' % Statement.split(DT.TAB_EQUAL_SPLIT, 1)[1].lstrip()
            SortedArch = DT.TAB_ARCH_COMMON
            if Statement.strip().startswith(DT.TAB_INF_DEFINES_CUSTOM_MAKEFILE):
                pos = Statement.find(DT.TAB_VALUE_SPLIT)
                if pos == -1:
                    pos = Statement.find(DT.TAB_EQUAL_SPLIT)
                Makefile = ConvertPath(Statement[pos + 1:].strip())
                Statement = Statement[:pos + 1] + ' ' + Makefile
            if SortedArch in NewSectionDict:
                NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
            else:
                NewSectionDict[SortedArch] = [Statement]
    SpecialStatementList = []

    # TAB_INF_DEFINES_INF_VERSION
    Statement = (u'%s ' % DT.TAB_INF_DEFINES_INF_VERSION).ljust(LeftOffset) + u'= %s' % '0x00010017'
    SpecialStatementList.append(Statement)

    # BaseName
    BaseName = ModuleObject.GetBaseName()
    if BaseName.startswith('.') or BaseName.startswith('-'):
        BaseName = '_' + BaseName
    Statement = (u'%s ' % DT.TAB_INF_DEFINES_BASE_NAME).ljust(LeftOffset) + u'= %s' % BaseName
    SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_FILE_GUID
    Statement = (u'%s ' % DT.TAB_INF_DEFINES_FILE_GUID).ljust(LeftOffset) + u'= %s' % ModuleObject.GetGuid()
    SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_VERSION_STRING
    Statement = (u'%s ' % DT.TAB_INF_DEFINES_VERSION_STRING).ljust(LeftOffset) + u'= %s' % ModuleObject.GetVersion()
    SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_VERSION_STRING
    if ModuleObject.UNIFlag:
        Statement = (u'%s ' % DT.TAB_INF_DEFINES_MODULE_UNI_FILE).ljust(LeftOffset) + \
                    u'= %s' % ModuleObject.GetModuleUniFile()
        SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_MODULE_TYPE
    if ModuleObject.GetModuleType():
        Statement = (u'%s ' % DT.TAB_INF_DEFINES_MODULE_TYPE).ljust(LeftOffset) + u'= %s' % ModuleObject.GetModuleType()
        SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_PCD_IS_DRIVER
    if ModuleObject.GetPcdIsDriver():
        Statement = (u'%s ' % DT.TAB_INF_DEFINES_PCD_IS_DRIVER).ljust(LeftOffset) + \
                    u'= %s' % ModuleObject.GetPcdIsDriver()
        SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION
    if ModuleObject.GetUefiSpecificationVersion():
        Statement = (u'%s ' % DT.TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION).ljust(LeftOffset) + \
                    u'= %s' % ModuleObject.GetUefiSpecificationVersion()
        SpecialStatementList.append(Statement)

    # TAB_INF_DEFINES_PI_SPECIFICATION_VERSION
    if ModuleObject.GetPiSpecificationVersion():
        Statement = (u'%s ' % DT.TAB_INF_DEFINES_PI_SPECIFICATION_VERSION).ljust(LeftOffset) + \
                    u'= %s' % ModuleObject.GetPiSpecificationVersion()
        SpecialStatementList.append(Statement)

    # LibraryClass
    for LibraryClass in ModuleObject.GetLibraryClassList():
        if LibraryClass.GetUsage() == DT.USAGE_ITEM_PRODUCES or \
           LibraryClass.GetUsage() == DT.USAGE_ITEM_SOMETIMES_PRODUCES:
            Statement = (u'%s ' % DT.TAB_INF_DEFINES_LIBRARY_CLASS).ljust(LeftOffset) + \
                        u'= %s' % LibraryClass.GetLibraryClass()
            if LibraryClass.GetSupModuleList():
                Statement += '|' + DT.TAB_SPACE_SPLIT.join(l for l in LibraryClass.GetSupModuleList())
            SpecialStatementList.append(Statement)

    # Spec Item
    for SpecItem in ModuleObject.GetSpecList():
        Spec, Version = SpecItem
        Spec = ConvertSpec(Spec)
        Statement = '%s %s = %s' % (DT.TAB_INF_DEFINES_SPEC, Spec, Version)
        SpecialStatementList.append(Statement)

    # Extern
    ExternList = []
    for Extern in ModuleObject.GetExternList():
        ArchList = Extern.GetSupArchList()
        EntryPoint = Extern.GetEntryPoint()
        UnloadImage = Extern.GetUnloadImage()
        Constructor = Extern.GetConstructor()
        Destructor = Extern.GetDestructor()
        HelpStringList = Extern.GetHelpTextList()
        FFE = Extern.GetFeatureFlag()
        ExternList.append([ArchList, EntryPoint, UnloadImage, Constructor, Destructor, FFE, HelpStringList])
    #
    # Add VALID_ARCHITECTURES information
    #
    ValidArchStatement = None
    if ModuleObject.SupArchList:
        ValidArchStatement = '\n' + '# ' + '\n'
        ValidArchStatement += '# The following information is for reference only and not required by the build tools.\n'
        ValidArchStatement += '# ' + '\n'
        ValidArchStatement += '# VALID_ARCHITECTURES = %s' % (' '.join(ModuleObject.SupArchList)) + '\n'
        ValidArchStatement += '# '
    if DT.TAB_ARCH_COMMON not in NewSectionDict:
        NewSectionDict[DT.TAB_ARCH_COMMON] = []
    NewSectionDict[DT.TAB_ARCH_COMMON] = NewSectionDict[DT.TAB_ARCH_COMMON] + SpecialStatementList
    GenMetaFileMisc.AddExternToDefineSec(NewSectionDict, DT.TAB_ARCH_COMMON, ExternList)
    if ValidArchStatement is not None:
        NewSectionDict[DT.TAB_ARCH_COMMON] = NewSectionDict[DT.TAB_ARCH_COMMON] + [ValidArchStatement]
    Content += GenSection('Defines', NewSectionDict)
    return Content

def GenLibraryClasses(ModuleObject):
    #
    # generate [LibraryClasses] section
    #
    Content = ''
    NewSectionDict = {}
    if not GlobalData.gIS_BINARY_INF:
        for LibraryClass in ModuleObject.GetLibraryClassList():
            if LibraryClass.GetUsage() == DT.USAGE_ITEM_PRODUCES:
                continue
            #
            # Generate generic comment
            #
            HelpTextList = LibraryClass.GetHelpTextList()
            HelpStr = _GetHelpStr(HelpTextList)
            CommentStr = GenGenericCommentF(HelpStr)
            Statement = CommentStr
            Name = LibraryClass.GetLibraryClass()
            FFE = LibraryClass.GetFeatureFlag()
            Statement += Name
            if FFE:
                Statement += '|' + FFE
            ModuleList = LibraryClass.GetSupModuleList()
            ArchList = LibraryClass.GetSupArchList()
            for Index in range(0, len(ArchList)):
                ArchList[Index] = ConvertArchForInstall(ArchList[Index])
            ArchList.sort()
            SortedArch = ' '.join(ArchList)
            KeyList = []
            if not ModuleList or IsAllModuleList(ModuleList):
                KeyList = [SortedArch]
            else:
                ModuleString = DT.TAB_VALUE_SPLIT.join(l for l in ModuleList)
                if not ArchList:
                    SortedArch = DT.TAB_ARCH_COMMON
                    KeyList = [SortedArch + '.' + ModuleString]
                else:
                    KeyList = [Arch + '.' + ModuleString for Arch in ArchList]
            for Key in KeyList:
                if Key in NewSectionDict:
                    NewSectionDict[Key] = NewSectionDict[Key] + [Statement]
                else:
                    NewSectionDict[Key] = [Statement]
        Content += GenSection('LibraryClasses', NewSectionDict)
    else:
        LibraryClassDict = {}
        for BinaryFile in ModuleObject.GetBinaryFileList():
            if not BinaryFile.AsBuiltList:
                continue
            for LibraryItem in BinaryFile.AsBuiltList[0].LibraryInstancesList:
                Statement = '# Guid: ' + LibraryItem.Guid + ' Version: ' + LibraryItem.Version

                if len(BinaryFile.SupArchList) == 0:
                    if 'COMMON' in LibraryClassDict and Statement not in LibraryClassDict['COMMON']:
                        LibraryClassDict['COMMON'].append(Statement)
                    else:
                        LibraryClassDict['COMMON'] = ['## @LIB_INSTANCES']
                        LibraryClassDict['COMMON'].append(Statement)
                else:
                    for Arch in BinaryFile.SupArchList:
                        if Arch in LibraryClassDict:
                            if Statement not in LibraryClassDict[Arch]:
                                LibraryClassDict[Arch].append(Statement)
                            else:
                                continue
                        else:
                            LibraryClassDict[Arch] = ['## @LIB_INSTANCES']
                            LibraryClassDict[Arch].append(Statement)
        Content += GenSection('LibraryClasses', LibraryClassDict)

    return Content

def GenPackages(ModuleObject):
    Content = ''
    #
    # generate [Packages] section
    #
    NewSectionDict = Sdict()
    WorkspaceDir = GlobalData.gWORKSPACE
    for PackageDependency in ModuleObject.GetPackageDependencyList():
        #
        # Generate generic comment
        #
        CommentStr = ''
        HelpText = PackageDependency.GetHelpText()
        if HelpText:
            HelpStr = HelpText.GetString()
            CommentStr = GenGenericCommentF(HelpStr)
        Statement = CommentStr
        Guid = PackageDependency.GetGuid()
        Version = PackageDependency.GetVersion()
        FFE = PackageDependency.GetFeatureFlag()
        Path = ''
        #
        # find package path/name
        #
        for PkgInfo in GlobalData.gWSPKG_LIST:
            if Guid == PkgInfo[1]:
                if (not Version) or (Version == PkgInfo[2]):
                    Path = PkgInfo[3]
                    break
        #
        # get relative path
        #
        RelaPath = GetRelativePath(Path, WorkspaceDir)
        Statement += RelaPath.replace('\\', '/')
        if FFE:
            Statement += '|' + FFE
        ArchList = sorted(PackageDependency.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]
    Content += GenSection('Packages', NewSectionDict)
    return Content

def GenSources(ModuleObject):
    #
    # generate [Sources] section
    #
    Content = ''
    NewSectionDict = {}
    for Source in ModuleObject.GetSourceFileList():
        SourceFile = Source.GetSourceFile()
        Family = Source.GetFamily()
        FeatureFlag = Source.GetFeatureFlag()
        SupArchList = sorted(Source.GetSupArchList())
        SortedArch = ' '.join(SupArchList)
        Statement = GenSourceStatement(ConvertPath(SourceFile), Family, FeatureFlag)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]
    Content += GenSection('Sources', NewSectionDict)

    return Content

def GenDepex(ModuleObject):
    #
    # generate [Depex] section
    #
    NewSectionDict = Sdict()
    Content = ''
    for Depex in ModuleObject.GetPeiDepex() + ModuleObject.GetDxeDepex() + ModuleObject.GetSmmDepex():
        HelpTextList = Depex.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr)
        SupArchList = Depex.GetSupArchList()
        SupModList = Depex.GetModuleType()
        Expression = Depex.GetDepex()
        Statement = CommentStr + Expression
        SupArchList.sort()
        KeyList = []
        if not SupArchList:
            SupArchList.append(DT.TAB_ARCH_COMMON.lower())
        if not SupModList:
            KeyList = SupArchList
        else:
            for ModuleType in SupModList:
                for Arch in SupArchList:
                    KeyList.append(ConvertArchForInstall(Arch) + '.' + ModuleType)
        for Key in KeyList:
            if Key in NewSectionDict:
                NewSectionDict[Key] = NewSectionDict[Key] + [Statement]
            else:
                NewSectionDict[Key] = [Statement]
    Content += GenSection('Depex', NewSectionDict, False)

    return Content
## GenUserExtensions
#
# GenUserExtensions
#
def GenUserExtensions(ModuleObject):
    NewSectionDict = {}
    for UserExtension in ModuleObject.GetUserExtensionList():
        if UserExtension.GetUserID() == DT.TAB_BINARY_HEADER_USERID and \
            UserExtension.GetIdentifier() == DT.TAB_BINARY_HEADER_IDENTIFIER:
            continue
        if UserExtension.GetIdentifier() == 'Depex':
            continue
        Statement = UserExtension.GetStatement()
# Comment the code to support user extension without any statement just the section header in []
#         if not Statement:
#             continue
        ArchList = UserExtension.GetSupArchList()
        for Index in range(0, len(ArchList)):
            ArchList[Index] = ConvertArchForInstall(ArchList[Index])
        ArchList.sort()
        KeyList = []
        CommonPreFix = ''
        if UserExtension.GetUserID():
            CommonPreFix = UserExtension.GetUserID()
            if CommonPreFix.find('.') > -1:
                CommonPreFix = '"' + CommonPreFix + '"'
            if UserExtension.GetIdentifier():
                CommonPreFix += '.' + '"' + UserExtension.GetIdentifier() + '"'
            if ArchList:
                KeyList = [CommonPreFix + '.' + Arch for Arch in ArchList]
            else:
                KeyList = [CommonPreFix]
        for Key in KeyList:
            if Key in NewSectionDict:
                NewSectionDict[Key] = NewSectionDict[Key] + [Statement]
            else:
                NewSectionDict[Key] = [Statement]
    Content = GenSection('UserExtensions', NewSectionDict, False)

    return Content

# GenSourceStatement
#
#  @param SourceFile: string of source file path/name
#  @param Family:     string of source file family field
#  @param FeatureFlag:  string of source file FeatureFlag field
#  @param TagName:  string of source file TagName field
#  @param ToolCode:  string of source file ToolCode field
#  @param HelpStr:  string of source file HelpStr field
#
#  @retval Statement: The generated statement for source
#
def GenSourceStatement(SourceFile, Family, FeatureFlag, TagName=None,
                       ToolCode=None, HelpStr=None):
    Statement = ''
    if HelpStr:
        Statement += GenGenericCommentF(HelpStr)
    #
    # format of SourceFile|Family|TagName|ToolCode|FeatureFlag
    #
    Statement += SourceFile
    if TagName is None:
        TagName = ''
    if ToolCode is None:
        ToolCode = ''
    if HelpStr is None:
        HelpStr = ''
    if FeatureFlag:
        Statement += '|' + Family + '|' + TagName + '|' + ToolCode + '|' + FeatureFlag
    elif ToolCode:
        Statement += '|' + Family + '|' + TagName + '|' + ToolCode
    elif TagName:
        Statement += '|' + Family + '|' + TagName
    elif Family:
        Statement += '|' + Family
    return Statement

# GenBinaryStatement
#
#  @param Key:       (FileName, FileType, FFE, SortedArch)
#  @param Value:     (Target, Family, TagName, Comment)
#
#
def GenBinaryStatement(Key, Value, SubTypeGuidValue=None):
    (FileName, FileType, FFE, SortedArch) = Key
    if SortedArch:
        pass
    if Value:
        (Target, Family, TagName, Comment) = Value
    else:
        Target = ''
        Family = ''
        TagName = ''
        Comment = ''
    if Comment:
        Statement = GenGenericCommentF(Comment)
    else:
        Statement = ''
    if FileType == 'SUBTYPE_GUID' and SubTypeGuidValue:
        Statement += FileType + '|' + SubTypeGuidValue + '|' + FileName
    else:
        Statement += FileType + '|' + FileName
    if FileType in DT.BINARY_FILE_TYPE_UI_LIST + DT.BINARY_FILE_TYPE_VER_LIST:
        if FFE:
            Statement += '|' + Target + '|' + FFE
        elif Target:
            Statement += '|' + Target
    else:
        if FFE:
            Statement += '|' + Target + '|' + Family + '|' + TagName + '|' + FFE
        elif TagName:
            Statement += '|' + Target + '|' + Family + '|' + TagName
        elif Family:
            Statement += '|' + Target + '|' + Family
        elif Target:
            Statement += '|' + Target
    return Statement
## GenGuidSections
#
#  @param GuidObjList: List of GuidObject
#  @retVal Content: The generated section contents
#
def GenGuidSections(GuidObjList):
    #
    # generate [Guids] section
    #
    Content = ''
    GuidDict = Sdict()
    for Guid in GuidObjList:
        HelpTextList = Guid.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CName = Guid.GetCName()
        FFE = Guid.GetFeatureFlag()
        Statement = CName
        if FFE:
            Statement += '|' + FFE
        Usage = Guid.GetUsage()
        GuidType = Guid.GetGuidTypeList()[0]
        VariableName = Guid.GetVariableName()
        #
        # Differentiate the generic comment and usage comment as multiple generic comment need to be put at first
        #
        if Usage == DT.ITEM_UNDEFINED and GuidType == DT.ITEM_UNDEFINED:
            # generate list of generic comment
            Comment = GenGenericCommentF(HelpStr)
        else:
            # generate list of other comment
            Comment = HelpStr.replace('\n', ' ')
            Comment = Comment.strip()
            if Comment:
                Comment = ' # ' + Comment
            else:
                Comment = ''
            if Usage != DT.ITEM_UNDEFINED and GuidType == DT.ITEM_UNDEFINED:
                Comment = '## ' + Usage + Comment
            elif GuidType == 'Variable':
                Comment = '## ' + Usage + ' ## ' + GuidType + ':' + VariableName + Comment
            else:
                Comment = '## ' + Usage + ' ## ' + GuidType + Comment

            if Comment:
                Comment += '\n'
        #
        # merge duplicate items
        #
        ArchList = sorted(Guid.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if (Statement, SortedArch) in GuidDict:
            PreviousComment = GuidDict[Statement, SortedArch]
            Comment = PreviousComment + Comment
        GuidDict[Statement, SortedArch] = Comment
    NewSectionDict = GenMetaFileMisc.TransferDict(GuidDict, 'INF_GUID')
    #
    # generate the section contents
    #
    if NewSectionDict:
        Content = GenSection('Guids', NewSectionDict)

    return Content

## GenProtocolPPiSections
#
#  @param ObjList: List of ProtocolObject or Ppi Object
#  @retVal Content: The generated section contents
#
def GenProtocolPPiSections(ObjList, IsProtocol):
    Content = ''
    Dict = Sdict()
    for Object in ObjList:
        HelpTextList = Object.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CName = Object.GetCName()
        FFE = Object.GetFeatureFlag()
        Statement = CName
        if FFE:
            Statement += '|' + FFE
        Usage = Object.GetUsage()
        Notify = Object.GetNotify()
        #
        # Differentiate the generic comment and usage comment as consecutive generic comment need to be put together
        #
        if Usage == DT.ITEM_UNDEFINED and Notify == '':
            # generate list of generic comment
            Comment = GenGenericCommentF(HelpStr)
        else:
            # generate list of other comment
            Comment = HelpStr.replace('\n', ' ')
            Comment = Comment.strip()
            if Comment:
                Comment = ' # ' + Comment
            else:
                Comment = ''
            if Usage == DT.ITEM_UNDEFINED and not Comment and Notify == '':
                Comment = ''
            else:
                if Notify:
                    Comment = '## ' + Usage + ' ## ' + 'NOTIFY' + Comment
                else:
                    Comment = '## ' + Usage + Comment
            if Comment:
                Comment += '\n'
        #
        # merge duplicate items
        #
        ArchList = sorted(Object.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if (Statement, SortedArch) in Dict:
            PreviousComment = Dict[Statement, SortedArch]
            Comment = PreviousComment + Comment
        Dict[Statement, SortedArch] = Comment
    NewSectionDict = GenMetaFileMisc.TransferDict(Dict, 'INF_PPI_PROTOCOL')
    #
    # generate the section contents
    #
    if NewSectionDict:
        if IsProtocol:
            Content = GenSection('Protocols', NewSectionDict)
        else:
            Content = GenSection('Ppis', NewSectionDict)

    return Content

## GenPcdSections
#
#
def GenPcdSections(ModuleObject):
    Content = ''
    if not GlobalData.gIS_BINARY_INF:
        #
        # for each Pcd Itemtype, maintain a dict so the same type will be grouped
        # together
        #
        ItemTypeDict = {}
        for Pcd in ModuleObject.GetPcdList():
            HelpTextList = Pcd.GetHelpTextList()
            HelpStr = _GetHelpStr(HelpTextList)
            Statement = ''
            CName = Pcd.GetCName()
            TokenSpaceGuidCName = Pcd.GetTokenSpaceGuidCName()
            DefaultValue = Pcd.GetDefaultValue()
            ItemType = Pcd.GetItemType()
            if ItemType in ItemTypeDict:
                Dict = ItemTypeDict[ItemType]
            else:
                Dict = Sdict()
                ItemTypeDict[ItemType] = Dict
            FFE = Pcd.GetFeatureFlag()
            Statement += TokenSpaceGuidCName + '.' + CName
            if DefaultValue:
                Statement += '|' + DefaultValue
                if FFE:
                    Statement += '|' + FFE
            elif FFE:
                Statement += '||' + FFE
            #
            # Generate comment
            #
            Usage = Pcd.GetValidUsage()
            # if FeatureFlag Pcd, then assume all Usage is CONSUMES
            if ItemType == DT.TAB_INF_FEATURE_PCD:
                Usage = DT.USAGE_ITEM_CONSUMES
            if Usage == DT.ITEM_UNDEFINED:
                # generate list of generic comment
                Comment = GenGenericCommentF(HelpStr)
            else:
                # generate list of other comment
                Comment = HelpStr.replace('\n', ' ')
                Comment = Comment.strip()
                if Comment:
                    Comment = ' # ' + Comment
                else:
                    Comment = ''
                Comment = '## ' + Usage + Comment
                if Comment:
                    Comment += '\n'
            #
            # Merge duplicate entries
            #
            ArchList = sorted(Pcd.GetSupArchList())
            SortedArch = ' '.join(ArchList)
            if (Statement, SortedArch) in Dict:
                PreviousComment = Dict[Statement, SortedArch]
                Comment = PreviousComment + Comment
            Dict[Statement, SortedArch] = Comment
        for ItemType in ItemTypeDict:
            # First we need to transfer the Dict to use SortedArch as key
            Dict = ItemTypeDict[ItemType]
            NewSectionDict = GenMetaFileMisc.TransferDict(Dict, 'INF_PCD')
            if NewSectionDict:
                Content += GenSection(ItemType, NewSectionDict)
    #
    # For AsBuild INF files
    #
    else:
        Content += GenAsBuiltPacthPcdSections(ModuleObject)
        Content += GenAsBuiltPcdExSections(ModuleObject)

    return Content

## GenPcdSections
#
#
def GenAsBuiltPacthPcdSections(ModuleObject):
    PatchPcdDict = {}
    for BinaryFile in ModuleObject.GetBinaryFileList():
        if not BinaryFile.AsBuiltList:
            continue
        for PatchPcd in BinaryFile.AsBuiltList[0].PatchPcdList:
            TokenSpaceName = ''
            PcdCName = PatchPcd.CName
            PcdValue = PatchPcd.DefaultValue
            PcdOffset = PatchPcd.Offset
            TokenSpaceGuidValue = PatchPcd.TokenSpaceGuidValue
            Token = PatchPcd.Token
            HelpTextList = PatchPcd.HelpTextList
            HelpString = ''
            for HelpStringItem in HelpTextList:
                for HelpLine in GetSplitValueList(HelpStringItem.String, '\n'):
                    HelpString += '## ' + HelpLine + '\n'
            TokenSpaceName, PcdCName = GenMetaFileMisc.ObtainPcdName(ModuleObject.PackageDependencyList,
                                                                     TokenSpaceGuidValue,
                                                                     Token)
            if TokenSpaceName == '' or PcdCName == '':
                Logger.Error("Upt",
                             ToolError.RESOURCE_NOT_AVAILABLE,
                             ST.ERR_INSTALL_FILE_DEC_FILE_ERROR % (TokenSpaceGuidValue, Token),
                             File=ModuleObject.GetFullPath())
            Statement = HelpString + TokenSpaceName + '.' + PcdCName + ' | ' + PcdValue + ' | ' + \
                         PcdOffset + DT.TAB_SPACE_SPLIT
            #
            # Use binary file's Arch to be Pcd's Arch
            #
            ArchList = []
            FileNameObjList = BinaryFile.GetFileNameList()
            if FileNameObjList:
                ArchList = FileNameObjList[0].GetSupArchList()
            if len(ArchList) == 0:
                if DT.TAB_ARCH_COMMON in PatchPcdDict:
                    if Statement not in PatchPcdDict[DT.TAB_ARCH_COMMON]:
                        PatchPcdDict[DT.TAB_ARCH_COMMON].append(Statement)
                else:
                    PatchPcdDict[DT.TAB_ARCH_COMMON] = [Statement]
            else:
                for Arch in ArchList:
                    if Arch in PatchPcdDict:
                        if Statement not in PatchPcdDict[Arch]:
                            PatchPcdDict[Arch].append(Statement)
                    else:
                        PatchPcdDict[Arch] = [Statement]
    return GenSection(DT.TAB_INF_PATCH_PCD, PatchPcdDict)
## GenPcdSections
#
#
def GenAsBuiltPcdExSections(ModuleObject):
    PcdExDict = {}
    for BinaryFile in ModuleObject.GetBinaryFileList():
        if not BinaryFile.AsBuiltList:
            continue
        for PcdExItem in BinaryFile.AsBuiltList[0].PcdExValueList:
            TokenSpaceName = ''
            PcdCName = PcdExItem.CName
            TokenSpaceGuidValue = PcdExItem.TokenSpaceGuidValue
            Token = PcdExItem.Token
            HelpTextList = PcdExItem.HelpTextList
            HelpString = ''
            for HelpStringItem in HelpTextList:
                for HelpLine in GetSplitValueList(HelpStringItem.String, '\n'):
                    HelpString += '## ' + HelpLine + '\n'
            TokenSpaceName, PcdCName = GenMetaFileMisc.ObtainPcdName(ModuleObject.PackageDependencyList,
                                                                     TokenSpaceGuidValue, Token)
            if TokenSpaceName == '' or PcdCName == '':
                Logger.Error("Upt",
                             ToolError.RESOURCE_NOT_AVAILABLE,
                             ST.ERR_INSTALL_FILE_DEC_FILE_ERROR % (TokenSpaceGuidValue, Token),
                             File=ModuleObject.GetFullPath())

            Statement = HelpString + TokenSpaceName + DT.TAB_SPLIT + PcdCName + DT.TAB_SPACE_SPLIT

            #
            # Use binary file's Arch to be Pcd's Arch
            #
            ArchList = []
            FileNameObjList = BinaryFile.GetFileNameList()
            if FileNameObjList:
                ArchList = FileNameObjList[0].GetSupArchList()

            if len(ArchList) == 0:
                if 'COMMON' in PcdExDict:
                    PcdExDict['COMMON'].append(Statement)
                else:
                    PcdExDict['COMMON'] = [Statement]
            else:
                for Arch in ArchList:
                    if Arch in PcdExDict:
                        if Statement not in PcdExDict[Arch]:
                            PcdExDict[Arch].append(Statement)
                    else:
                        PcdExDict[Arch] = [Statement]
    return GenSection('PcdEx', PcdExDict)

## GenSpecialSections
#  generate special sections for Event/BootMode/Hob
#
def GenSpecialSections(ObjectList, SectionName, UserExtensionsContent=''):
    #
    # generate section
    #
    Content = ''
    NewSectionDict = {}
    for Obj in ObjectList:
        #
        # Generate comment
        #
        CommentStr = ''
        HelpTextList = Obj.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr)
        if SectionName == 'Hob':
            Type = Obj.GetHobType()
        elif SectionName == 'Event':
            Type = Obj.GetEventType()
        elif SectionName == 'BootMode':
            Type = Obj.GetSupportedBootModes()
        else:
            assert(SectionName)
        Usage = Obj.GetUsage()

        # If the content already in UserExtensionsContent then ignore
        if '[%s]' % SectionName in UserExtensionsContent and Type in UserExtensionsContent:
            return ''

        Statement = ' ' + Type + ' ## ' + Usage
        if CommentStr in ['#\n', '#\n#\n']:
            CommentStr = '#\n#\n#\n'
        #
        # the first head comment line should start with '##\n', if it starts with '#\n', then add one '#'
        # else add '##\n' to meet the format defined in INF spec
        #
        if CommentStr.startswith('#\n'):
            CommentStr = '#' + CommentStr
        elif CommentStr:
            CommentStr = '##\n' + CommentStr
        if CommentStr and not CommentStr.endswith('\n#\n'):
            CommentStr = CommentStr + '#\n'
        NewStateMent = CommentStr + Statement
        SupArch = sorted(Obj.GetSupArchList())
        SortedArch = ' '.join(SupArch)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [NewStateMent]
        else:
            NewSectionDict[SortedArch] = [NewStateMent]
    SectionContent = GenSection(SectionName, NewSectionDict)
    SectionContent = SectionContent.strip()
    if SectionContent:
        Content = '# ' + ('\n' + '# ').join(GetSplitValueList(SectionContent, '\n'))
        Content = Content.lstrip()
    #
    # add a return to differentiate it between other possible sections
    #
    if Content:
        Content += '\n'
    return Content
## GenBuildOptions
#
#
def GenBuildOptions(ModuleObject):
    Content = ''
    if not ModuleObject.BinaryModule:
        #
        # generate [BuildOptions] section
        #
        NewSectionDict = {}
        for UserExtension in ModuleObject.GetUserExtensionList():
            BuildOptionDict = UserExtension.GetBuildOptionDict()
            if not BuildOptionDict:
                continue
            for Arch in BuildOptionDict:
                if Arch in NewSectionDict:
                    NewSectionDict[Arch] = NewSectionDict[Arch] + [BuildOptionDict[Arch]]
                else:
                    NewSectionDict[Arch] = [BuildOptionDict[Arch]]
        Content = GenSection('BuildOptions', NewSectionDict)
    else:
        BuildOptionDict = {}
        for BinaryFile in ModuleObject.GetBinaryFileList():
            if not BinaryFile.AsBuiltList:
                continue
            for BuilOptionItem in BinaryFile.AsBuiltList[0].BinaryBuildFlagList:
                Statement = '#' + BuilOptionItem.AsBuiltOptionFlags
                if len(BinaryFile.SupArchList) == 0:
                    if 'COMMON' in BuildOptionDict:
                        if Statement not in BuildOptionDict['COMMON']:
                            BuildOptionDict['COMMON'].append(Statement)
                    else:
                        BuildOptionDict['COMMON'] = ['## @AsBuilt']
                        BuildOptionDict['COMMON'].append(Statement)
                else:
                    for Arch in BinaryFile.SupArchList:
                        if Arch in BuildOptionDict:
                            if Statement not in BuildOptionDict[Arch]:
                                BuildOptionDict[Arch].append(Statement)
                        else:
                            BuildOptionDict[Arch] = ['## @AsBuilt']
                            BuildOptionDict[Arch].append(Statement)
        Content = GenSection('BuildOptions', BuildOptionDict)

    return Content
## GenBinaries
#
#
def GenBinaries(ModuleObject):
    NewSectionDict = {}
    BinariesDict = []
    for UserExtension in ModuleObject.GetUserExtensionList():
        BinariesDict = UserExtension.GetBinariesDict()
        if BinariesDict:
            break
    for BinaryFile in ModuleObject.GetBinaryFileList():
        FileNameObjList = BinaryFile.GetFileNameList()
        for FileNameObj in FileNameObjList:
            FileName = ConvertPath(FileNameObj.GetFilename())
            FileType = FileNameObj.GetFileType()
            FFE = FileNameObj.GetFeatureFlag()
            ArchList = sorted(FileNameObj.GetSupArchList())
            SortedArch = ' '.join(ArchList)
            Key = (FileName, FileType, FFE, SortedArch)
            if Key in BinariesDict:
                ValueList = BinariesDict[Key]
                for ValueItem in ValueList:
                    Statement = GenBinaryStatement(Key, ValueItem)
                    if SortedArch in NewSectionDict:
                        NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
                    else:
                        NewSectionDict[SortedArch] = [Statement]
                #
                # as we already generated statement for this DictKey here set the Valuelist to be empty
                # to avoid generate duplicate entries as the DictKey may have multiple entries
                #
                BinariesDict[Key] = []
            else:
                if FileType == 'SUBTYPE_GUID' and FileNameObj.GetGuidValue():
                    Statement = GenBinaryStatement(Key, None, FileNameObj.GetGuidValue())
                else:
                    Statement = GenBinaryStatement(Key, None)
                if SortedArch in NewSectionDict:
                    NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
                else:
                    NewSectionDict[SortedArch] = [Statement]
    Content = GenSection('Binaries', NewSectionDict)

    return Content
