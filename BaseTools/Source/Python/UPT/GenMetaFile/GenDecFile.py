## @file GenDecFile.py
#
# This file contained the logical of transfer package object to DEC files.
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
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
GenDEC
'''

from Library.Parsing import GenSection
from Library.CommentGenerating import GenHeaderCommentSection
from Library.CommentGenerating import GenGenericCommentF
from Library.CommentGenerating import GenDecTailComment
from Library.CommentGenerating import _GetHelpStr
from Library.Misc import GuidStringToGuidStructureString
from Library.Misc import SaveFileOnChange
from Library.Misc import ConvertPath
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_COMMA_SPLIT
from Library.DataType import TAB_ARCH_COMMON
from Library.DataType import TAB_DEC_DEFINES_DEC_SPECIFICATION
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_NAME
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_GUID
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_VERSION


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

        PcdErrList = Pcd.GetPcdErrorsList()
        if PcdErrList:
            CommentStr += GenPcdErrComment(PcdErrList[0])
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

        ArchList = Pcd.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]            
        
    for ValidUsage in ValidUsageDict:
        Content += GenSection(ValidUsage, ValidUsageDict[ValidUsage])
    
    return Content

def GenGuidProtocolPpi(Package, Content):
    #
    # generate [Guids] section
    #
    NewSectionDict = {}
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
        Statement += CName + '  =  ' + Value
        #
        # generate tail comment
        #
        if Guid.GetSupModuleList():
            Statement += GenDecTailComment(Guid.GetSupModuleList())     
        ArchList = Guid.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]            

    Content += GenSection('Guids', NewSectionDict)    
 
    #
    # generate [Protocols] section
    #
    NewSectionDict = {}
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
        Statement += CName + '  =  ' + Value

        #
        # generate tail comment
        #
        if Protocol.GetSupModuleList():
            Statement += GenDecTailComment(Protocol.GetSupModuleList())
        ArchList = Protocol.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]            

    Content += GenSection('Protocols', NewSectionDict) 

    #
    # generate [Ppis] section
    #
    NewSectionDict = {}
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
        Statement += CName + '  =  ' + Value

        #
        # generate tail comment
        #
        if Ppi.GetSupModuleList():
            Statement += GenDecTailComment(Ppi.GetSupModuleList())
        ArchList = Ppi.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]            

    Content += GenSection('Ppis', NewSectionDict)
    
    return Content

## Transfer Package Object to Dec files
#
# Transfer all contents of a standard Package Object to a Dec file 
#
# @param Package:  A Package 
#
def PackageToDec(Package):
    #
    # Init global information for the file
    #
    ContainerFile = Package.GetFullPath()
    
    Content = ''
    #
    # generate header comment section
    #
    Content += GenHeaderCommentSection(Package.GetAbstract(), \
                                       Package.GetDescription(), \
                                       Package.GetCopyright(), \
                                       Package.GetLicense())
    
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
    NewSectionDict = {TAB_ARCH_COMMON : []}
    SpecialItemList = []
    
    Statement = '%s = %s' % (TAB_DEC_DEFINES_DEC_SPECIFICATION, '0x00010017')
    SpecialItemList.append(Statement)
    
    BaseName = Package.GetBaseName()
    if BaseName.startswith('.') or BaseName.startswith('-'):
        BaseName = '_' + BaseName
    Statement = '%s = %s' % (TAB_DEC_DEFINES_PACKAGE_NAME, BaseName)
    SpecialItemList.append(Statement)
    Statement = '%s = %s' % (TAB_DEC_DEFINES_PACKAGE_VERSION, Package.GetVersion())
    SpecialItemList.append(Statement)
    Statement = '%s = %s' % (TAB_DEC_DEFINES_PACKAGE_GUID, Package.GetGuid())
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
            HelpStr = '@libraryclass ' + HelpStr
        CommentStr = GenGenericCommentF(HelpStr, 2)

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
        ArchList = LibraryClass.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = \
            NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]            

    Content += GenSection('LibraryClasses', NewSectionDict)

    Content = GenPcd(Package, Content)
    
    #
    # generate [UserExtensions] section
    #
    NewSectionDict = {}
    for UserExtension in Package.GetUserExtensionList():
        Statement = UserExtension.GetStatement()
        if not Statement:
            continue
        
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
        Content += '\n\n' + SectionName + Statement

    SaveFileOnChange(ContainerFile, Content, False)
    return ContainerFile

## GenPcdErrComment
#
#  @param PcdErrObject:  PcdErrorObject
#  
#  @retval CommentStr:   Generated comment lines, with prefix "#"
# 
def GenPcdErrComment (PcdErrObject):
    EndOfLine = "\n"    
    ValidValueRange = PcdErrObject.GetValidValueRange()
    if ValidValueRange:
        CommentStr = "# @ValidRange " + ValidValueRange + EndOfLine
    
    ValidValue = PcdErrObject.GetValidValue()
    if ValidValue:
        ValidValueList = \
        [Value for Value in ValidValue.split(TAB_SPACE_SPLIT) if Value]
        CommentStr = \
        "# @ValidList " + TAB_COMMA_SPLIT.join(ValidValueList) + EndOfLine
        
    Expression = PcdErrObject.GetExpression()
    if Expression:
        CommentStr = "# @Expression " + Expression + EndOfLine
        
    return CommentStr

