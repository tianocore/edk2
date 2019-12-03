## @file
# This file is used to define comment generating interface
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
CommentGenerating
'''

##
# Import Modules
#
from Library.StringUtils import GetSplitValueList
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_INF_GUIDTYPE_VAR
from Library.DataType import USAGE_ITEM_NOTIFY
from Library.DataType import ITEM_UNDEFINED
from Library.DataType import TAB_HEADER_COMMENT
from Library.DataType import TAB_BINARY_HEADER_COMMENT
from Library.DataType import TAB_COMMENT_SPLIT
from Library.DataType import TAB_SPECIAL_COMMENT
from Library.DataType import END_OF_LINE
from Library.DataType import TAB_COMMENT_EDK1_SPLIT
from Library.DataType import TAB_COMMENT_EDK1_START
from Library.DataType import TAB_COMMENT_EDK1_END
from Library.DataType import TAB_STAR
from Library.DataType import TAB_PCD_PROMPT
from Library.UniClassObject import ConvertSpecialUnicodes
from Library.Misc import GetLocalValue
## GenTailCommentLines
#
# @param TailCommentLines:  the tail comment lines that need to be generated
# @param LeadingSpaceNum:   the number of leading space needed for non-first
#                            line tail comment
#
def GenTailCommentLines (TailCommentLines, LeadingSpaceNum = 0):
    TailCommentLines = TailCommentLines.rstrip(END_OF_LINE)
    CommentStr = TAB_SPACE_SPLIT*2 + TAB_SPECIAL_COMMENT + TAB_SPACE_SPLIT + \
    (END_OF_LINE + LeadingSpaceNum * TAB_SPACE_SPLIT + TAB_SPACE_SPLIT*2 + TAB_SPECIAL_COMMENT + \
     TAB_SPACE_SPLIT).join(GetSplitValueList(TailCommentLines, END_OF_LINE))

    return CommentStr

## GenGenericComment
#
# @param CommentLines:   Generic comment Text, maybe Multiple Lines
#
def GenGenericComment (CommentLines):
    if not CommentLines:
        return ''
    CommentLines = CommentLines.rstrip(END_OF_LINE)
    CommentStr = TAB_SPECIAL_COMMENT + TAB_SPACE_SPLIT + (END_OF_LINE + TAB_COMMENT_SPLIT + TAB_SPACE_SPLIT).join\
    (GetSplitValueList(CommentLines, END_OF_LINE)) + END_OF_LINE
    return CommentStr

## GenGenericCommentF
#
#  similar to GenGenericComment but will remove <EOL> at end of comment once,
#  and for line with only <EOL>, '#\n' will be generated instead of '# \n'
#
# @param CommentLines:   Generic comment Text, maybe Multiple Lines
# @return CommentStr:    Generated comment line
#
def GenGenericCommentF (CommentLines, NumOfPound=1, IsPrompt=False, IsInfLibraryClass=False):
    if not CommentLines:
        return ''
    #
    # if comment end with '\n', then remove it to prevent one extra line
    # generate later on
    #
    if CommentLines.endswith(END_OF_LINE):
        CommentLines = CommentLines[:-1]
    CommentStr = ''
    if IsPrompt:
        CommentStr += TAB_COMMENT_SPLIT * NumOfPound + TAB_SPACE_SPLIT + TAB_PCD_PROMPT + TAB_SPACE_SPLIT + \
        CommentLines.replace(END_OF_LINE, '') + END_OF_LINE
    else:
        CommentLineList = GetSplitValueList(CommentLines, END_OF_LINE)
        FindLibraryClass = False
        for Line in CommentLineList:
            # If this comment is for @libraryclass and it has multiple lines
            # make sure the second lines align to the first line after @libraryclass as below
            #
            # ## @libraryclass XYZ FIRST_LINE
            # ##               ABC SECOND_LINE
            #
            if IsInfLibraryClass and Line.find(u'@libraryclass ') > -1:
                FindLibraryClass = True
            if Line == '':
                CommentStr += TAB_COMMENT_SPLIT * NumOfPound + END_OF_LINE
            else:
                if FindLibraryClass and Line.find(u'@libraryclass ') > -1:
                    CommentStr += TAB_COMMENT_SPLIT * NumOfPound + TAB_SPACE_SPLIT + Line + END_OF_LINE
                elif FindLibraryClass:
                    CommentStr += TAB_COMMENT_SPLIT * NumOfPound + TAB_SPACE_SPLIT * 16 + Line + END_OF_LINE
                else:
                    CommentStr += TAB_COMMENT_SPLIT * NumOfPound + TAB_SPACE_SPLIT + Line + END_OF_LINE

    return CommentStr


## GenHeaderCommentSection
#
# Generate Header comment sections
#
# @param Abstract      One line of abstract
# @param Description   multiple lines of Description
# @param Copyright     possible multiple copyright lines
# @param License       possible multiple license lines
#
def GenHeaderCommentSection(Abstract, Description, Copyright, License, IsBinaryHeader=False, \
                            CommChar=TAB_COMMENT_SPLIT):
    Content = ''

    #
    # Convert special character to (c), (r) and (tm).
    #
    Abstract = ConvertSpecialUnicodes(Abstract)
    Description = ConvertSpecialUnicodes(Description)
    if IsBinaryHeader:
        Content += CommChar * 2 + TAB_SPACE_SPLIT + TAB_BINARY_HEADER_COMMENT + '\r\n'
    elif CommChar == TAB_COMMENT_EDK1_SPLIT:
        Content += CommChar + TAB_SPACE_SPLIT + TAB_COMMENT_EDK1_START + TAB_STAR + TAB_SPACE_SPLIT +\
         TAB_HEADER_COMMENT + '\r\n'
    else:
        Content += CommChar * 2 + TAB_SPACE_SPLIT + TAB_HEADER_COMMENT + '\r\n'
    if Abstract:
        Abstract = Abstract.rstrip('\r\n')
        Content += CommChar + TAB_SPACE_SPLIT + ('\r\n' + CommChar + TAB_SPACE_SPLIT).join(GetSplitValueList\
                                                                                                (Abstract, '\n'))
        Content += '\r\n' + CommChar + '\r\n'
    else:
        Content += CommChar + '\r\n'

    if Description:
        Description = Description.rstrip('\r\n')
        Content += CommChar + TAB_SPACE_SPLIT + ('\r\n' + CommChar + TAB_SPACE_SPLIT).join(GetSplitValueList\
                                                  (Description, '\n'))
        Content += '\r\n' + CommChar + '\r\n'

    #
    # There is no '#\n' line to separate multiple copyright lines in code base
    #
    if Copyright:
        Copyright = Copyright.rstrip('\r\n')
        Content += CommChar + TAB_SPACE_SPLIT + ('\r\n' + CommChar + TAB_SPACE_SPLIT).join\
        (GetSplitValueList(Copyright, '\n'))
        Content += '\r\n' + CommChar + '\r\n'

    if License:
        License = License.rstrip('\r\n')
        Content += CommChar + TAB_SPACE_SPLIT + ('\r\n' + CommChar + TAB_SPACE_SPLIT).join(GetSplitValueList\
                                                  (License, '\n'))
        Content += '\r\n' + CommChar + '\r\n'

    if CommChar == TAB_COMMENT_EDK1_SPLIT:
        Content += CommChar + TAB_SPACE_SPLIT + TAB_STAR + TAB_COMMENT_EDK1_END + '\r\n'
    else:
        Content += CommChar * 2 + '\r\n'

    return Content


## GenInfPcdTailComment
#  Generate Pcd tail comment for Inf, this would be one line comment
#
# @param Usage:            Usage type
# @param TailCommentText:  Comment text for tail comment
#
def GenInfPcdTailComment (Usage, TailCommentText):
    if (Usage == ITEM_UNDEFINED) and (not TailCommentText):
        return ''

    CommentLine = TAB_SPACE_SPLIT.join([Usage, TailCommentText])
    return GenTailCommentLines(CommentLine)

## GenInfProtocolPPITailComment
#  Generate Protocol/PPI tail comment for Inf
#
# @param Usage:            Usage type
# @param TailCommentText:  Comment text for tail comment
#
def GenInfProtocolPPITailComment (Usage, Notify, TailCommentText):
    if (not Notify) and (Usage == ITEM_UNDEFINED) and (not TailCommentText):
        return ''

    if Notify:
        CommentLine = USAGE_ITEM_NOTIFY + " ## "
    else:
        CommentLine = ''

    CommentLine += TAB_SPACE_SPLIT.join([Usage, TailCommentText])
    return GenTailCommentLines(CommentLine)

## GenInfGuidTailComment
#  Generate Guid tail comment for Inf
#
# @param Usage:            Usage type
# @param TailCommentText:  Comment text for tail comment
#
def GenInfGuidTailComment (Usage, GuidTypeList, VariableName, TailCommentText):
    GuidType = GuidTypeList[0]
    if (Usage == ITEM_UNDEFINED) and (GuidType == ITEM_UNDEFINED) and \
        (not TailCommentText):
        return ''

    FirstLine = Usage + " ## " + GuidType
    if GuidType == TAB_INF_GUIDTYPE_VAR:
        FirstLine += ":" + VariableName

    CommentLine = TAB_SPACE_SPLIT.join([FirstLine, TailCommentText])
    return GenTailCommentLines(CommentLine)

## GenDecGuidTailComment
#
# @param SupModuleList:  Supported module type list
#
def GenDecTailComment (SupModuleList):
    CommentLine = TAB_SPACE_SPLIT.join(SupModuleList)
    return GenTailCommentLines(CommentLine)


## _GetHelpStr
#  get HelpString from a list of HelpTextObject, the priority refer to
#  related HLD
#
#  @param HelpTextObjList: List of HelpTextObject
#
#  @return HelpStr: the help text string found, '' means no help text found
#
def _GetHelpStr(HelpTextObjList):
    ValueList = []
    for HelpObj in HelpTextObjList:
        ValueList.append((HelpObj.GetLang(), HelpObj.GetString()))
    return GetLocalValue(ValueList, True)
