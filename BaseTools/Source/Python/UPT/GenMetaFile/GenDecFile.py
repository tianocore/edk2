## @file GenDecFile.py
#
# This file contained the logical of transfer package object to DEC files.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
GenDEC
'''
import os
import stat
import codecs
from hashlib import md5
from Core.FileHook import __FileHookOpen__
from Library.Parsing import GenSection
from Library.CommentGenerating import GenHeaderCommentSection
from Library.CommentGenerating import GenGenericCommentF
from Library.CommentGenerating import GenDecTailComment
from Library.CommentGenerating import _GetHelpStr
from Library.Misc import GuidStringToGuidStructureString
from Library.Misc import SaveFileOnChange
from Library.Misc import ConvertPath
from Library.Misc import GetLocalValue
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_COMMA_SPLIT
from Library.DataType import END_OF_LINE
from Library.DataType import TAB_ARCH_COMMON
from Library.DataType import TAB_VALUE_SPLIT
from Library.DataType import TAB_COMMENT_SPLIT
from Library.DataType import TAB_PCD_VALIDRANGE
from Library.DataType import TAB_PCD_VALIDLIST
from Library.DataType import TAB_PCD_EXPRESSION
from Library.DataType import TAB_DEC_DEFINES_DEC_SPECIFICATION
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_NAME
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_GUID
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_VERSION
from Library.DataType import TAB_DEC_DEFINES_PKG_UNI_FILE
from Library.DataType import TAB_DEC_PACKAGE_ABSTRACT
from Library.DataType import TAB_DEC_PACKAGE_DESCRIPTION
from Library.DataType import TAB_DEC_BINARY_ABSTRACT
from Library.DataType import TAB_DEC_BINARY_DESCRIPTION
from Library.DataType import TAB_LANGUAGE_EN_X
from Library.DataType import TAB_BINARY_HEADER_USERID
from Library.DataType import TAB_BINARY_HEADER_IDENTIFIER
from Library.DataType import TAB_COMMENT_EDK1_SPLIT
from Library.DataType import TAB_ENCODING_UTF16LE
from Library.DataType import TAB_CAPHEX_START
from Library.DataType import TAB_HEX_START
from Library.DataType import TAB_UNDERLINE_SPLIT
from Library.DataType import TAB_STR_TOKENERR
from Library.DataType import TAB_STR_TOKENCNAME
from Library.DataType import TAB_PCD_ERROR_SECTION_COMMENT
from Library.DataType import TAB_PCD_ERROR
from Library.DataType import TAB_SECTION_START
from Library.DataType import TAB_SECTION_END
from Library.DataType import TAB_SPLIT
import Library.DataType as DT
from Library.UniClassObject import FormatUniEntry
from Library.StringUtils import GetUniFileName

def GenPcd(Package, Content):
    #
    # generate [Pcd] section
    # <TokenSpcCName>.<TokenCName>|<Value>|<DatumType>|<Token>
    #
    ValidUsageDict = {}
    for Pcd in Package.GetPcdList():
        #
        # Generate generic comment
        #
        HelpTextList = Pcd.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr, 2)

        PromptList = Pcd.GetPromptList()
        PromptStr = _GetHelpStr(PromptList)
        CommentStr += GenGenericCommentF(PromptStr.strip(), 1, True)

        PcdErrList = Pcd.GetPcdErrorsList()
        for PcdErr in PcdErrList:
            CommentStr += GenPcdErrComment(PcdErr)
        Statement = CommentStr

        CName = Pcd.GetCName()
        TokenSpaceGuidCName = Pcd.GetTokenSpaceGuidCName()
        DefaultValue = Pcd.GetDefaultValue()
        DatumType = Pcd.GetDatumType()
        Token = Pcd.GetToken()
        ValidUsage = Pcd.GetValidUsage()

        if ValidUsage == 'FeaturePcd':
            ValidUsage = 'PcdsFeatureFlag'
        elif ValidUsage == 'PatchPcd':
            ValidUsage = 'PcdsPatchableInModule'
        elif ValidUsage == 'FixedPcd':
            ValidUsage = 'PcdsFixedAtBuild'
        elif ValidUsage == 'Pcd':
            ValidUsage = 'PcdsDynamic'
        elif ValidUsage == 'PcdEx':
            ValidUsage = 'PcdsDynamicEx'

        if ValidUsage in ValidUsageDict:
            NewSectionDict = ValidUsageDict[ValidUsage]
        else:
            NewSectionDict = {}
            ValidUsageDict[ValidUsage] = NewSectionDict
        Statement += TokenSpaceGuidCName + '.' + CName
        Statement += '|' + DefaultValue
        Statement += '|' + DatumType
        Statement += '|' + Token
        #
        # generate tail comment
        #
        if Pcd.GetSupModuleList():
            Statement += GenDecTailComment(Pcd.GetSupModuleList())

        ArchList = sorted(Pcd.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]

    for ValidUsage in ValidUsageDict:
        Content += GenSection(ValidUsage, ValidUsageDict[ValidUsage], True, True)

    return Content

def GenPcdErrorMsgSection(Package, Content):
    if not Package.PcdErrorCommentDict:
        return Content

    #
    # Generate '# [Error.<TokenSpcCName>]' section
    #
    Content += END_OF_LINE + END_OF_LINE
    SectionComment = TAB_COMMENT_SPLIT + END_OF_LINE
    SectionComment += TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT + TAB_PCD_ERROR_SECTION_COMMENT + END_OF_LINE
    SectionComment += TAB_COMMENT_SPLIT + END_OF_LINE
    TokenSpcCNameList = []

    #
    # Get TokenSpcCName list in PcdErrorCommentDict in Package object
    #
    for (TokenSpcCName, ErrorNumber) in Package.PcdErrorCommentDict:
        if TokenSpcCName not in TokenSpcCNameList:
            TokenSpcCNameList.append(TokenSpcCName)

    for TokenSpcCNameItem in TokenSpcCNameList:
        SectionName = TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT + TAB_SECTION_START + TAB_PCD_ERROR + \
                      TAB_SPLIT + TokenSpcCNameItem + TAB_SECTION_END + END_OF_LINE
        Content += SectionComment
        Content += SectionName
        for (TokenSpcCName, ErrorNumber) in Package.PcdErrorCommentDict:
            if TokenSpcCNameItem == TokenSpcCName:
                PcdErrorMsg = GetLocalValue(Package.PcdErrorCommentDict[(TokenSpcCName, ErrorNumber)])
                SectionItem = TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT + TAB_SPACE_SPLIT + \
                              ErrorNumber + TAB_SPACE_SPLIT + TAB_VALUE_SPLIT + TAB_SPACE_SPLIT + \
                              PcdErrorMsg + END_OF_LINE
                Content += SectionItem

    Content += TAB_COMMENT_SPLIT
    return Content

def GenGuidProtocolPpi(Package, Content):
    #
    # generate [Guids] section
    #
    NewSectionDict = {}

    LeftOffset = 46
    # Get the line offset need
    # If the real one < the min one, use the min one
    # else use the real one
    for Guid in Package.GetGuidList():
        if len(Guid.GetCName()) > LeftOffset:
            LeftOffset = len(Guid.GetCName())

    # Generate
    for Guid in Package.GetGuidList():
        #
        # Generate generic comment
        #
        HelpTextList = Guid.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr, 2)

        Statement = CommentStr
        CName = Guid.GetCName()
        Value = GuidStringToGuidStructureString(Guid.GetGuid())
        Statement += CName.ljust(LeftOffset) + ' = ' + Value
        #
        # generate tail comment
        #
        if Guid.GetSupModuleList():
            Statement += GenDecTailComment(Guid.GetSupModuleList())
        ArchList = sorted(Guid.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]

    Content += GenSection('Guids', NewSectionDict, True, True)

    #
    # generate [Protocols] section
    #
    NewSectionDict = {}
    LeftOffset = 46
    # Get the line offset need
    # If the real one < the min one, use the min one
    # else use the real one
    for Protocol in Package.GetProtocolList():
        if len(Protocol.GetCName()) > LeftOffset:
            LeftOffset = len(Protocol.GetCName())

    for Protocol in Package.GetProtocolList():
        #
        # Generate generic comment
        #
        HelpTextList = Protocol.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr, 2)

        Statement = CommentStr
        CName = Protocol.GetCName()
        Value = GuidStringToGuidStructureString(Protocol.GetGuid())
        Statement += CName.ljust(LeftOffset) + ' = ' + Value

        #
        # generate tail comment
        #
        if Protocol.GetSupModuleList():
            Statement += GenDecTailComment(Protocol.GetSupModuleList())
        ArchList = sorted(Protocol.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]

    Content += GenSection('Protocols', NewSectionDict, True, True)

    #
    # generate [Ppis] section
    #
    NewSectionDict = {}
    LeftOffset = 46
    # Get the line offset need
    # If the real one < the min one, use the min one
    # else use the real one
    for Ppi in Package.GetPpiList():
        if len(Ppi.GetCName()) > LeftOffset:
            LeftOffset = len(Ppi.GetCName())

    for Ppi in Package.GetPpiList():
        #
        # Generate generic comment
        #
        HelpTextList = Ppi.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr, 2)

        Statement = CommentStr
        CName = Ppi.GetCName()
        Value = GuidStringToGuidStructureString(Ppi.GetGuid())
        Statement += CName.ljust(LeftOffset) + ' = ' + Value

        #
        # generate tail comment
        #
        if Ppi.GetSupModuleList():
            Statement += GenDecTailComment(Ppi.GetSupModuleList())
        ArchList = sorted(Ppi.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]

    Content += GenSection('Ppis', NewSectionDict, True, True)

    return Content

## Transfer Package Object to Dec files
#
# Transfer all contents of a standard Package Object to a Dec file
#
# @param Package:  A Package
#
def PackageToDec(Package, DistHeader = None):
    #
    # Init global information for the file
    #
    ContainerFile = Package.GetFullPath()

    Content = ''

    #
    # Generate file header
    #
    PackageAbstract = GetLocalValue(Package.GetAbstract())
    PackageDescription = GetLocalValue(Package.GetDescription())
    PackageCopyright = ''
    PackageLicense = ''
    for (Lang, Copyright) in Package.GetCopyright():
        if Lang:
            pass
        PackageCopyright = Copyright
    for (Lang, License) in Package.GetLicense():
        if Lang:
            pass
        PackageLicense = License
    if not PackageAbstract and DistHeader:
        PackageAbstract = GetLocalValue(DistHeader.GetAbstract())
    if not PackageDescription and DistHeader:
        PackageDescription = GetLocalValue(DistHeader.GetDescription())
    if not PackageCopyright and DistHeader:
        for (Lang, Copyright) in DistHeader.GetCopyright():
            PackageCopyright = Copyright
    if not PackageLicense and DistHeader:
        for (Lang, License) in DistHeader.GetLicense():
            PackageLicense = License

    #
    # Generate header comment section of DEC file
    #
    Content += GenHeaderCommentSection(PackageAbstract, \
                                       PackageDescription, \
                                       PackageCopyright, \
                                       PackageLicense).replace('\r\n', '\n')

    #
    # Generate Binary header
    #
    for UserExtension in Package.GetUserExtensionList():
        if UserExtension.GetUserID() == TAB_BINARY_HEADER_USERID \
        and UserExtension.GetIdentifier() == TAB_BINARY_HEADER_IDENTIFIER:
            PackageBinaryAbstract = GetLocalValue(UserExtension.GetBinaryAbstract())
            PackageBinaryDescription = GetLocalValue(UserExtension.GetBinaryDescription())
            PackageBinaryCopyright = ''
            PackageBinaryLicense = ''
            for (Lang, Copyright) in UserExtension.GetBinaryCopyright():
                PackageBinaryCopyright = Copyright
            for (Lang, License) in UserExtension.GetBinaryLicense():
                PackageBinaryLicense = License
            if PackageBinaryAbstract and PackageBinaryDescription and \
            PackageBinaryCopyright and PackageBinaryLicense:
                Content += GenHeaderCommentSection(PackageBinaryAbstract,
                                           PackageBinaryDescription,
                                           PackageBinaryCopyright,
                                           PackageBinaryLicense,
                                           True)

    #
    # Generate PACKAGE_UNI_FILE for the Package
    #
    FileHeader = GenHeaderCommentSection(PackageAbstract, PackageDescription, PackageCopyright, PackageLicense, False, \
                                         TAB_COMMENT_EDK1_SPLIT)
    GenPackageUNIEncodeFile(Package, FileHeader)

    #
    # for each section, maintain a dict, sorted arch will be its key,
    #statement list will be its data
    # { 'Arch1 Arch2 Arch3': [statement1, statement2],
    #   'Arch1' : [statement1, statement3]
    #  }
    #

    #
    # generate [Defines] section
    #
    LeftOffset = 31
    NewSectionDict = {TAB_ARCH_COMMON : []}
    SpecialItemList = []

    Statement = (u'%s ' % TAB_DEC_DEFINES_DEC_SPECIFICATION).ljust(LeftOffset) + u'= %s' % '0x00010017'
    SpecialItemList.append(Statement)

    BaseName = Package.GetBaseName()
    if BaseName.startswith('.') or BaseName.startswith('-'):
        BaseName = '_' + BaseName
    Statement = (u'%s ' % TAB_DEC_DEFINES_PACKAGE_NAME).ljust(LeftOffset) + u'= %s' % BaseName
    SpecialItemList.append(Statement)

    Statement = (u'%s ' % TAB_DEC_DEFINES_PACKAGE_VERSION).ljust(LeftOffset) + u'= %s' % Package.GetVersion()
    SpecialItemList.append(Statement)

    Statement = (u'%s ' % TAB_DEC_DEFINES_PACKAGE_GUID).ljust(LeftOffset) + u'= %s' % Package.GetGuid()
    SpecialItemList.append(Statement)

    if Package.UNIFlag:
        Statement = (u'%s ' % TAB_DEC_DEFINES_PKG_UNI_FILE).ljust(LeftOffset) + u'= %s' % Package.GetBaseName() + '.uni'
        SpecialItemList.append(Statement)

    for SortedArch in NewSectionDict:
        NewSectionDict[SortedArch] = \
        NewSectionDict[SortedArch] + SpecialItemList
    Content += GenSection('Defines', NewSectionDict)

    #
    # generate [Includes] section
    #
    NewSectionDict = {}
    IncludeArchList = Package.GetIncludeArchList()
    if IncludeArchList:
        for Path, ArchList in IncludeArchList:
            Statement = Path
            ArchList.sort()
            SortedArch = ' '.join(ArchList)
            if SortedArch in NewSectionDict:
                NewSectionDict[SortedArch] = \
                NewSectionDict[SortedArch] + [ConvertPath(Statement)]
            else:
                NewSectionDict[SortedArch] = [ConvertPath(Statement)]

    Content += GenSection('Includes', NewSectionDict)

    #
    # generate [guids][protocols][ppis] sections
    #
    Content = GenGuidProtocolPpi(Package, Content)

    #
    # generate [LibraryClasses] section
    #
    NewSectionDict = {}
    for LibraryClass in Package.GetLibraryClassList():
        #
        # Generate generic comment
        #
        HelpTextList = LibraryClass.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        if HelpStr:
            HelpStr = '@libraryclass' + HelpStr
        CommentStr = GenGenericCommentF(HelpStr, 2, False, True)

        Statement = CommentStr
        Name = LibraryClass.GetLibraryClass()
        IncludeHeader = LibraryClass.GetIncludeHeader()
        Statement += Name + '|' + ConvertPath(IncludeHeader)
        #
        # generate tail comment
        #
        if LibraryClass.GetSupModuleList():
            Statement += \
            GenDecTailComment(LibraryClass.GetSupModuleList())
        ArchList = sorted(LibraryClass.GetSupArchList())
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]

    Content += GenSection('LibraryClasses', NewSectionDict, True, True)

    #
    # Generate '# [Error.<TokenSpcCName>]' section
    #
    Content = GenPcdErrorMsgSection(Package, Content)

    Content = GenPcd(Package, Content)

    #
    # generate [UserExtensions] section
    #
    NewSectionDict = {}
    for UserExtension in Package.GetUserExtensionList():
        if UserExtension.GetUserID() == TAB_BINARY_HEADER_USERID and \
            UserExtension.GetIdentifier() == TAB_BINARY_HEADER_IDENTIFIER:
            continue

        # Generate Private Section first
        if UserExtension.GetUserID() == DT.TAB_INTEL and UserExtension.GetIdentifier() == DT.TAB_PRIVATE:
            Content += '\n' + UserExtension.GetStatement()
            continue

        Statement = UserExtension.GetStatement()
        if not Statement:
            continue
        else:
            LineList = Statement.split('\n')
            NewStatement = ""
            for Line in LineList:
                NewStatement += "  %s\n" % Line

        SectionList = []
        SectionName = 'UserExtensions'
        UserId = UserExtension.GetUserID()
        if UserId:
            if '.' in UserId:
                UserId = '"' + UserId + '"'
            SectionName += '.' + UserId
            if UserExtension.GetIdentifier():
                SectionName += '.' + '"' + UserExtension.GetIdentifier() + '"'
        if not UserExtension.GetSupArchList():
            SectionList.append(SectionName)
        else:
            for Arch in UserExtension.GetSupArchList():
                SectionList.append(SectionName + '.' + Arch)
        SectionName = ', '.join(SectionList)
        SectionName = ''.join(['[', SectionName, ']\n'])
        Content += '\n' + SectionName + NewStatement

    SaveFileOnChange(ContainerFile, Content, False)
    if DistHeader.ReadOnly:
        os.chmod(ContainerFile, stat.S_IRUSR|stat.S_IRGRP|stat.S_IROTH)
    else:
        os.chmod(ContainerFile, stat.S_IRUSR|stat.S_IRGRP|stat.S_IROTH|stat.S_IWUSR|stat.S_IWGRP|stat.S_IWOTH)
    return ContainerFile

## GenPackageUNIEncodeFile
# GenPackageUNIEncodeFile, default is a UCS-2LE encode file
#
def GenPackageUNIEncodeFile(PackageObject, UniFileHeader = '', Encoding=TAB_ENCODING_UTF16LE):
    GenUNIFlag = False
    OnlyLANGUAGE_EN_X = True
    BinaryAbstract = []
    BinaryDescription = []
    #
    # If more than one language code is used for any element that would be present in the PACKAGE_UNI_FILE,
    # then the PACKAGE_UNI_FILE must be created.
    #
    for (Key, Value) in PackageObject.GetAbstract() + PackageObject.GetDescription():
        if Key == TAB_LANGUAGE_EN_X:
            GenUNIFlag = True
        else:
            OnlyLANGUAGE_EN_X = False

    for UserExtension in PackageObject.GetUserExtensionList():
        if UserExtension.GetUserID() == TAB_BINARY_HEADER_USERID \
        and UserExtension.GetIdentifier() == TAB_BINARY_HEADER_IDENTIFIER:
            for (Key, Value) in UserExtension.GetBinaryAbstract():
                if Key == TAB_LANGUAGE_EN_X:
                    GenUNIFlag = True
                else:
                    OnlyLANGUAGE_EN_X = False
                BinaryAbstract.append((Key, Value))

            for (Key, Value) in UserExtension.GetBinaryDescription():
                if Key == TAB_LANGUAGE_EN_X:
                    GenUNIFlag = True
                else:
                    OnlyLANGUAGE_EN_X = False
                BinaryDescription.append((Key, Value))

    for Pcd in PackageObject.GetPcdList():
        for TxtObj in Pcd.GetPromptList() + Pcd.GetHelpTextList():
            if TxtObj.GetLang() == TAB_LANGUAGE_EN_X:
                GenUNIFlag = True
            else:
                OnlyLANGUAGE_EN_X = False

        for PcdError in Pcd.GetPcdErrorsList():
            if PcdError.GetErrorNumber().startswith('0x') or PcdError.GetErrorNumber().startswith('0X'):
                for (Key, Value) in PcdError.GetErrorMessageList():
                    if Key == TAB_LANGUAGE_EN_X:
                        GenUNIFlag = True
                    else:
                        OnlyLANGUAGE_EN_X = False
    if not GenUNIFlag:
        return
    elif OnlyLANGUAGE_EN_X:
        return
    else:
        PackageObject.UNIFlag = True

    if not os.path.exists(os.path.dirname(PackageObject.GetFullPath())):
        os.makedirs(os.path.dirname(PackageObject.GetFullPath()))

    ContainerFile = GetUniFileName(os.path.dirname(PackageObject.GetFullPath()), PackageObject.GetBaseName())

    Content = UniFileHeader + '\r\n'
    Content += '\r\n'

    Content += FormatUniEntry('#string ' + TAB_DEC_PACKAGE_ABSTRACT, PackageObject.GetAbstract(), ContainerFile) + '\r\n'

    Content += FormatUniEntry('#string ' + TAB_DEC_PACKAGE_DESCRIPTION, PackageObject.GetDescription(), ContainerFile) \
    + '\r\n'

    Content += FormatUniEntry('#string ' + TAB_DEC_BINARY_ABSTRACT, BinaryAbstract, ContainerFile) + '\r\n'

    Content += FormatUniEntry('#string ' + TAB_DEC_BINARY_DESCRIPTION, BinaryDescription, ContainerFile) + '\r\n'

    PromptGenList = []
    HelpTextGenList = []
    for Pcd in PackageObject.GetPcdList():
        # Generate Prompt for each Pcd
        PcdPromptStrName = '#string ' + 'STR_' + Pcd.GetTokenSpaceGuidCName() + '_' + Pcd.GetCName() + '_PROMPT '
        TokenValueList = []
        for TxtObj in Pcd.GetPromptList():
            Lang = TxtObj.GetLang()
            PromptStr = TxtObj.GetString()
            #
            # Avoid generating the same PROMPT entry more than one time.
            #
            if (PcdPromptStrName, Lang) not in PromptGenList:
                TokenValueList.append((Lang, PromptStr))
                PromptGenList.append((PcdPromptStrName, Lang))
        PromptString = FormatUniEntry(PcdPromptStrName, TokenValueList, ContainerFile) + '\r\n'
        if PromptString not in Content:
            Content += PromptString

        # Generate Help String for each Pcd
        PcdHelpStrName = '#string ' + 'STR_' + Pcd.GetTokenSpaceGuidCName() + '_' + Pcd.GetCName() + '_HELP '
        TokenValueList = []
        for TxtObj in Pcd.GetHelpTextList():
            Lang = TxtObj.GetLang()
            HelpStr = TxtObj.GetString()
            #
            # Avoid generating the same HELP entry more than one time.
            #
            if (PcdHelpStrName, Lang) not in HelpTextGenList:
                TokenValueList.append((Lang, HelpStr))
                HelpTextGenList.append((PcdHelpStrName, Lang))
        HelpTextString = FormatUniEntry(PcdHelpStrName, TokenValueList, ContainerFile) + '\r\n'
        if HelpTextString not in Content:
            Content += HelpTextString

        # Generate PcdError for each Pcd if ErrorNo exist.
        for PcdError in Pcd.GetPcdErrorsList():
            ErrorNo = PcdError.GetErrorNumber()
            if ErrorNo.startswith(TAB_HEX_START) or ErrorNo.startswith(TAB_CAPHEX_START):
                PcdErrStrName = '#string ' + TAB_STR_TOKENCNAME + TAB_UNDERLINE_SPLIT + Pcd.GetTokenSpaceGuidCName() \
                    + TAB_UNDERLINE_SPLIT + TAB_STR_TOKENERR \
                    + TAB_UNDERLINE_SPLIT + ErrorNo[2:]
                PcdErrString = FormatUniEntry(PcdErrStrName, PcdError.GetErrorMessageList(), ContainerFile) + '\r\n'
                if PcdErrString not in Content:
                    Content += PcdErrString

    File = codecs.open(ContainerFile, 'w', Encoding)
    File.write(u'\uFEFF' + Content)
    File.stream.close()
    Md5Signature = md5(__FileHookOpen__(str(ContainerFile), 'rb').read())
    Md5Sum = Md5Signature.hexdigest()
    if (ContainerFile, Md5Sum) not in PackageObject.FileList:
        PackageObject.FileList.append((ContainerFile, Md5Sum))

    return ContainerFile

## GenPcdErrComment
#
#  @param PcdErrObject:  PcdErrorObject
#
#  @retval CommentStr:   Generated comment lines, with prefix "#"
#
def GenPcdErrComment (PcdErrObject):
    CommentStr = ''
    ErrorCode = PcdErrObject.GetErrorNumber()
    ValidValueRange = PcdErrObject.GetValidValueRange()
    if ValidValueRange:
        CommentStr = TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT + TAB_PCD_VALIDRANGE + TAB_SPACE_SPLIT
        if ErrorCode:
            CommentStr += ErrorCode + TAB_SPACE_SPLIT + TAB_VALUE_SPLIT + TAB_SPACE_SPLIT
        CommentStr += ValidValueRange + END_OF_LINE

    ValidValue = PcdErrObject.GetValidValue()
    if ValidValue:
        ValidValueList = \
        [Value for Value in ValidValue.split(TAB_SPACE_SPLIT) if Value]
        CommentStr = TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT + TAB_PCD_VALIDLIST + TAB_SPACE_SPLIT
        if ErrorCode:
            CommentStr += ErrorCode + TAB_SPACE_SPLIT + TAB_VALUE_SPLIT + TAB_SPACE_SPLIT
        CommentStr += TAB_COMMA_SPLIT.join(ValidValueList) + END_OF_LINE

    Expression = PcdErrObject.GetExpression()
    if Expression:
        CommentStr = TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT + TAB_PCD_EXPRESSION + TAB_SPACE_SPLIT
        if ErrorCode:
            CommentStr += ErrorCode + TAB_SPACE_SPLIT + TAB_VALUE_SPLIT + TAB_SPACE_SPLIT
        CommentStr += Expression + END_OF_LINE

    return CommentStr

