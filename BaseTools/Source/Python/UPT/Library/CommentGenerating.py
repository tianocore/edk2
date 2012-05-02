## @file
# This file is used to define comment generating interface
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
CommentGenerating
'''

##
# Import Modules
#
from Library.String import GetSplitValueList
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_INF_GUIDTYPE_VAR
from Library.DataType import USAGE_ITEM_NOTIFY
from Library.DataType import ITEM_UNDEFINED
from Library.DataType import LANGUAGE_EN_US

## GenTailCommentLines
#
# @param TailCommentLines:  the tail comment lines that need to be generated
# @param LeadingSpaceNum:   the number of leading space needed for non-first 
#                            line tail comment
# 
def GenTailCommentLines (TailCommentLines, LeadingSpaceNum = 0):
    EndOfLine = "\n"
    TailCommentLines = TailCommentLines.rstrip(EndOfLine)
    CommentStr = "  ## " + (EndOfLine + LeadingSpaceNum * TAB_SPACE_SPLIT + \
                            "  ## ").join(GetSplitValueList(TailCommentLines, \
                                                            EndOfLine))
    return CommentStr

## GenGenericComment
#
# @param CommentLines:   Generic comment Text, maybe Multiple Lines
# 
def GenGenericComment (CommentLines):
    if not CommentLines:
        return ''
    EndOfLine = "\n"
    CommentLines = CommentLines.rstrip(EndOfLine)
    CommentStr = '## ' + (EndOfLine + '# ').join\
    (GetSplitValueList(CommentLines, EndOfLine)) + EndOfLine
    return CommentStr

## GenGenericCommentF
#
#  similar to GenGenericComment but will remove <EOL> at end of comment once,
#  and for line with only <EOL>, '#\n' will be generated instead of '# \n'
#
# @param CommentLines:   Generic comment Text, maybe Multiple Lines
# @return CommentStr:    Generated comment line 
# 
def GenGenericCommentF (CommentLines, NumOfPound=1):
    if not CommentLines:
        return ''
    EndOfLine = "\n"
    #
    # if comment end with '\n', then remove it to prevent one extra line
    # generate later on
    #
    if CommentLines.endswith(EndOfLine):
        CommentLines = CommentLines[:-1]
    CommentLineList = GetSplitValueList(CommentLines, EndOfLine)
    CommentStr = ''
    for Line in CommentLineList:
        if Line == '':
            CommentStr += '#' * NumOfPound + '\n'
        else:
            CommentStr += '#' * NumOfPound + ' ' + Line + '\n'
     
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
def GenHeaderCommentSection(Abstract, Description, Copyright, License):
    EndOfLine = '\n'
    Content = ''
    
    Content += '## @file' + EndOfLine
    if Abstract:
        Abstract = Abstract.rstrip(EndOfLine)
        Content += '# ' + Abstract + EndOfLine
        Content += '#' + EndOfLine
    else:
        Content += '#' + EndOfLine

    if Description:
        Description = Description.rstrip(EndOfLine)
        Content += '# ' + (EndOfLine + '# ').join(GetSplitValueList\
                                                  (Description, '\n'))
        Content += EndOfLine + '#' + EndOfLine        
  
    #
    # There is no '#\n' line to separate multiple copyright lines in code base 
    #
    if Copyright:
        Copyright = Copyright.rstrip(EndOfLine)
        Content += '# ' + (EndOfLine + '# ').join\
        (GetSplitValueList(Copyright, '\n'))
        Content += EndOfLine + '#' + EndOfLine

    if License:
        License = License.rstrip(EndOfLine)
        Content += '# ' + (EndOfLine + '# ').join(GetSplitValueList\
                                                  (License, '\n'))
        Content += EndOfLine + '#' + EndOfLine

    Content += '##' + EndOfLine
    
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
    HelpStr = ''

    for HelpObj in HelpTextObjList:
        if HelpObj and HelpObj.GetLang() == LANGUAGE_EN_US:
            HelpStr = HelpObj.GetString()
            return HelpStr
    
    for HelpObj in HelpTextObjList:
        if HelpObj and HelpObj.GetLang().startswith('en'):
            HelpStr = HelpObj.GetString()
            return HelpStr

    for HelpObj in HelpTextObjList:
        if HelpObj and not HelpObj.GetLang():
            HelpStr = HelpObj.GetString()
            return HelpStr
    
    return HelpStr