## @file
# This file is used to define comment parsing interface
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
CommentParsing
'''

##
# Import Modules
#
import re

from Library.String import GetSplitValueList
from Library.String import CleanString2
from Library.DataType import HEADER_COMMENT_NOT_STARTED
from Library.DataType import TAB_COMMENT_SPLIT
from Library.DataType import HEADER_COMMENT_LICENSE
from Library.DataType import HEADER_COMMENT_ABSTRACT
from Library.DataType import HEADER_COMMENT_COPYRIGHT
from Library.DataType import HEADER_COMMENT_DESCRIPTION
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_COMMA_SPLIT
from Library.DataType import SUP_MODULE_LIST
from Library.DataType import TAB_VALUE_SPLIT
from Library.DataType import TAB_PCD_VALIDRANGE
from Library.DataType import TAB_PCD_VALIDLIST
from Library.DataType import TAB_PCD_EXPRESSION
from Library.DataType import TAB_PCD_PROMPT
from Library.DataType import TAB_CAPHEX_START
from Library.DataType import TAB_HEX_START
from Library.DataType import PCD_ERR_CODE_MAX_SIZE
from Library.ExpressionValidate import IsValidRangeExpr
from Library.ExpressionValidate import IsValidListExpr
from Library.ExpressionValidate import IsValidLogicalExpr
from Object.POM.CommonObject import TextObject
from Object.POM.CommonObject import PcdErrorObject
import Logger.Log as Logger
from Logger.ToolError import FORMAT_INVALID
from Logger.ToolError import FORMAT_NOT_SUPPORTED
from Logger import StringTable as ST

## ParseHeaderCommentSection
#
# Parse Header comment section lines, extract Abstract, Description, Copyright
# , License lines
#
# @param CommentList:   List of (Comment, LineNumber)
# @param FileName:      FileName of the comment
#
def ParseHeaderCommentSection(CommentList, FileName = None, IsBinaryHeader = False):
    Abstract = ''
    Description = ''
    Copyright = ''
    License = ''
    EndOfLine = "\n"
    if IsBinaryHeader:
        STR_HEADER_COMMENT_START = "@BinaryHeader"
    else:
        STR_HEADER_COMMENT_START = "@file"
    HeaderCommentStage = HEADER_COMMENT_NOT_STARTED
    
    #
    # first find the last copyright line
    #
    Last = 0
    for Index in xrange(len(CommentList)-1, 0, -1):
        Line = CommentList[Index][0]
        if _IsCopyrightLine(Line):
            Last = Index
            break
    
    for Item in CommentList:
        Line = Item[0]
        LineNo = Item[1]
        
        if not Line.startswith(TAB_COMMENT_SPLIT) and Line:
            Logger.Error("\nUPT", FORMAT_INVALID, ST.ERR_INVALID_COMMENT_FORMAT, FileName, Item[1])
        Comment = CleanString2(Line)[1]
        Comment = Comment.strip()
        #
        # if there are blank lines between License or Description, keep them as they would be 
        # indication of different block; or in the position that Abstract should be, also keep it
        # as it indicates that no abstract
        #
        if not Comment and HeaderCommentStage not in [HEADER_COMMENT_LICENSE, \
                                                      HEADER_COMMENT_DESCRIPTION, HEADER_COMMENT_ABSTRACT]:
            continue
        
        if HeaderCommentStage == HEADER_COMMENT_NOT_STARTED:
            if Comment.startswith(STR_HEADER_COMMENT_START):
                HeaderCommentStage = HEADER_COMMENT_ABSTRACT
            else:
                License += Comment + EndOfLine
        else:
            if HeaderCommentStage == HEADER_COMMENT_ABSTRACT:
                #
                # in case there is no abstract and description
                #
                if not Comment:
                    HeaderCommentStage = HEADER_COMMENT_DESCRIPTION
                elif _IsCopyrightLine(Comment):
                    Result, ErrMsg = _ValidateCopyright(Comment)
                    ValidateCopyright(Result, ST.WRN_INVALID_COPYRIGHT, FileName, LineNo, ErrMsg)
                    Copyright += Comment + EndOfLine
                    HeaderCommentStage = HEADER_COMMENT_COPYRIGHT
                else:                    
                    Abstract += Comment + EndOfLine
                    HeaderCommentStage = HEADER_COMMENT_DESCRIPTION
            elif HeaderCommentStage == HEADER_COMMENT_DESCRIPTION:
                #
                # in case there is no description
                #                
                if _IsCopyrightLine(Comment):
                    Result, ErrMsg = _ValidateCopyright(Comment)
                    ValidateCopyright(Result, ST.WRN_INVALID_COPYRIGHT, FileName, LineNo, ErrMsg)
                    Copyright += Comment + EndOfLine
                    HeaderCommentStage = HEADER_COMMENT_COPYRIGHT
                else:
                    Description += Comment + EndOfLine                
            elif HeaderCommentStage == HEADER_COMMENT_COPYRIGHT:
                if _IsCopyrightLine(Comment):
                    Result, ErrMsg = _ValidateCopyright(Comment)
                    ValidateCopyright(Result, ST.WRN_INVALID_COPYRIGHT, FileName, LineNo, ErrMsg)
                    Copyright += Comment + EndOfLine
                else:
                    #
                    # Contents after copyright line are license, those non-copyright lines in between
                    # copyright line will be discarded 
                    #
                    if LineNo > Last:
                        if License:
                            License += EndOfLine
                        License += Comment + EndOfLine
                        HeaderCommentStage = HEADER_COMMENT_LICENSE                
            else:
                if not Comment and not License:
                    continue
                License += Comment + EndOfLine
                  
    return Abstract.strip(), Description.strip(), Copyright.strip(), License.strip()

## _IsCopyrightLine
# check whether current line is copyright line, the criteria is whether there is case insensitive keyword "Copyright" 
# followed by zero or more white space characters followed by a "(" character 
#
# @param LineContent:  the line need to be checked
# @return: True if current line is copyright line, False else
#
def _IsCopyrightLine (LineContent):
    LineContent = LineContent.upper()
    Result = False
    
    ReIsCopyrightRe = re.compile(r"""(^|\s)COPYRIGHT *\(""", re.DOTALL)
    if ReIsCopyrightRe.search(LineContent):
        Result = True

    return Result

## ParseGenericComment
#
# @param GenericComment: Generic comment list, element of 
#                        (CommentLine, LineNum)
# @param ContainerFile:  Input value for filename of Dec file
# 
def ParseGenericComment (GenericComment, ContainerFile=None, SkipTag=None):
    if ContainerFile:
        pass
    HelpTxt = None         
    HelpStr = '' 
        
    for Item in GenericComment:
        CommentLine = Item[0]
        Comment = CleanString2(CommentLine)[1]
        if SkipTag is not None and Comment.startswith(SkipTag):
            Comment = Comment.replace(SkipTag, '', 1)
        HelpStr += Comment + '\n'
        
    if HelpStr:
        HelpTxt = TextObject()
        if HelpStr.endswith('\n') and not HelpStr.endswith('\n\n') and HelpStr != '\n':
            HelpStr = HelpStr[:-1]
        HelpTxt.SetString(HelpStr)

    return HelpTxt

## ParsePcdErrorCode
#
# @param Value: original ErrorCode value 
# @param ContainerFile: Input value for filename of Dec file
# @param LineNum: Line Num 
# 
def ParsePcdErrorCode (Value = None, ContainerFile = None, LineNum = None):      
    try: 
        if Value.strip().startswith((TAB_HEX_START, TAB_CAPHEX_START)):
            Base = 16
        else:
            Base = 10
        ErrorCode = long(Value, Base)
        if ErrorCode > PCD_ERR_CODE_MAX_SIZE or ErrorCode < 0:
            Logger.Error('Parser', 
                        FORMAT_NOT_SUPPORTED,
                        "The format %s of ErrorCode is not valid, should be UNIT32 type or long type" % Value,
                        File = ContainerFile, 
                        Line = LineNum)
        #
        # To delete the tailing 'L'
        #
        return hex(ErrorCode)[:-1]
    except ValueError, XStr:
        if XStr:
            pass
        Logger.Error('Parser', 
                    FORMAT_NOT_SUPPORTED,
                    "The format %s of ErrorCode is not valid, should be UNIT32 type or long type" % Value,
                    File = ContainerFile, 
                    Line = LineNum)
    
## ParseDecPcdGenericComment
#
# @param GenericComment: Generic comment list, element of (CommentLine, 
#                         LineNum)
# @param ContainerFile:  Input value for filename of Dec file
# 
def ParseDecPcdGenericComment (GenericComment, ContainerFile, TokenSpaceGuidCName, CName, MacroReplaceDict):       
    HelpStr = '' 
    PromptStr = ''
    PcdErr = None
    PcdErrList = []
    ValidValueNum = 0
    ValidRangeNum = 0
    ExpressionNum = 0
        
    for (CommentLine, LineNum) in GenericComment:
        Comment = CleanString2(CommentLine)[1]
        #
        # To replace Macro
        #
        MACRO_PATTERN = '[\t\s]*\$\([A-Z][_A-Z0-9]*\)'
        MatchedStrs =  re.findall(MACRO_PATTERN, Comment)
        for MatchedStr in MatchedStrs:
            if MatchedStr:
                Macro = MatchedStr.strip().lstrip('$(').rstrip(')').strip()
                if Macro in MacroReplaceDict:
                    Comment = Comment.replace(MatchedStr, MacroReplaceDict[Macro])    
        if Comment.startswith(TAB_PCD_VALIDRANGE):
            if ValidValueNum > 0 or ExpressionNum > 0:
                Logger.Error('Parser', 
                             FORMAT_NOT_SUPPORTED,
                             ST.WRN_MULTI_PCD_RANGES,
                             File = ContainerFile, 
                             Line = LineNum)
            else:
                PcdErr = PcdErrorObject()
                PcdErr.SetTokenSpaceGuidCName(TokenSpaceGuidCName)
                PcdErr.SetCName(CName)
                PcdErr.SetFileLine(Comment)
                PcdErr.SetLineNum(LineNum)
                ValidRangeNum += 1
            ValidRange = Comment.replace(TAB_PCD_VALIDRANGE, "", 1).strip()
            Valid, Cause = _CheckRangeExpression(ValidRange)
            if Valid:
                ValueList = ValidRange.split(TAB_VALUE_SPLIT)
                if len(ValueList) > 1:
                    PcdErr.SetValidValueRange((TAB_VALUE_SPLIT.join(ValueList[1:])).strip())
                    PcdErr.SetErrorNumber(ParsePcdErrorCode(ValueList[0], ContainerFile, LineNum))
                else:
                    PcdErr.SetValidValueRange(ValidRange)
                PcdErrList.append(PcdErr)
            else:
                Logger.Error("Parser",
                         FORMAT_NOT_SUPPORTED,
                         Cause, 
                         ContainerFile, 
                         LineNum)
        elif Comment.startswith(TAB_PCD_VALIDLIST):
            if ValidRangeNum > 0 or ExpressionNum > 0:
                Logger.Error('Parser', 
                             FORMAT_NOT_SUPPORTED,
                             ST.WRN_MULTI_PCD_RANGES,
                             File = ContainerFile, 
                             Line = LineNum)
            elif ValidValueNum > 0:
                Logger.Error('Parser', 
                             FORMAT_NOT_SUPPORTED,
                             ST.WRN_MULTI_PCD_VALIDVALUE,
                             File = ContainerFile, 
                             Line = LineNum)
            else:
                PcdErr = PcdErrorObject()
                PcdErr.SetTokenSpaceGuidCName(TokenSpaceGuidCName)
                PcdErr.SetCName(CName)
                PcdErr.SetFileLine(Comment)
                PcdErr.SetLineNum(LineNum)
                ValidValueNum += 1
                ValidValueExpr = Comment.replace(TAB_PCD_VALIDLIST, "", 1).strip()
            Valid, Cause = _CheckListExpression(ValidValueExpr)
            if Valid:
                ValidValue = Comment.replace(TAB_PCD_VALIDLIST, "", 1).replace(TAB_COMMA_SPLIT, TAB_SPACE_SPLIT)
                ValueList = ValidValue.split(TAB_VALUE_SPLIT)
                if len(ValueList) > 1:
                    PcdErr.SetValidValue((TAB_VALUE_SPLIT.join(ValueList[1:])).strip())
                    PcdErr.SetErrorNumber(ParsePcdErrorCode(ValueList[0], ContainerFile, LineNum))
                else:
                    PcdErr.SetValidValue(ValidValue)
                PcdErrList.append(PcdErr)
            else:
                Logger.Error("Parser",
                         FORMAT_NOT_SUPPORTED,
                         Cause, 
                         ContainerFile, 
                         LineNum)
        elif Comment.startswith(TAB_PCD_EXPRESSION):
            if ValidRangeNum > 0 or ValidValueNum > 0:
                Logger.Error('Parser', 
                             FORMAT_NOT_SUPPORTED,
                             ST.WRN_MULTI_PCD_RANGES,
                             File = ContainerFile, 
                             Line = LineNum)
            else:
                PcdErr = PcdErrorObject()
                PcdErr.SetTokenSpaceGuidCName(TokenSpaceGuidCName)
                PcdErr.SetCName(CName)
                PcdErr.SetFileLine(Comment)
                PcdErr.SetLineNum(LineNum)
                ExpressionNum += 1
            Expression = Comment.replace(TAB_PCD_EXPRESSION, "", 1).strip()
            Valid, Cause = _CheckExpression(Expression)
            if Valid:
                ValueList = Expression.split(TAB_VALUE_SPLIT)
                if len(ValueList) > 1:
                    PcdErr.SetExpression((TAB_VALUE_SPLIT.join(ValueList[1:])).strip())
                    PcdErr.SetErrorNumber(ParsePcdErrorCode(ValueList[0], ContainerFile, LineNum))
                else:
                    PcdErr.SetExpression(Expression)
                PcdErrList.append(PcdErr)
            else:        
                Logger.Error("Parser",
                         FORMAT_NOT_SUPPORTED,
                         Cause, 
                         ContainerFile, 
                         LineNum)                
        elif Comment.startswith(TAB_PCD_PROMPT):
            if PromptStr:
                Logger.Error('Parser', 
                             FORMAT_NOT_SUPPORTED,
                             ST.WRN_MULTI_PCD_PROMPT,
                             File = ContainerFile, 
                             Line = LineNum)
            PromptStr = Comment.replace(TAB_PCD_PROMPT, "", 1).strip()
        else:
            if Comment:
                HelpStr += Comment + '\n'
    
    #
    # remove the last EOL if the comment is of format 'FOO\n'
    #
    if HelpStr.endswith('\n'):
        if HelpStr != '\n' and not HelpStr.endswith('\n\n'):
            HelpStr = HelpStr[:-1]

    return HelpStr, PcdErrList, PromptStr

## ParseDecPcdTailComment
#
# @param TailCommentList:    Tail comment list of Pcd, item of format (Comment, LineNum)
# @param ContainerFile:      Input value for filename of Dec file
# @retVal SupModuleList:  The supported module type list detected
# @retVal HelpStr:  The generic help text string detected
#
def ParseDecPcdTailComment (TailCommentList, ContainerFile):
    assert(len(TailCommentList) == 1)
    TailComment = TailCommentList[0][0]
    LineNum = TailCommentList[0][1]

    Comment = TailComment.lstrip(" #")
    
    ReFindFirstWordRe = re.compile(r"""^([^ #]*)""", re.DOTALL)
    
    #
    # get first word and compare with SUP_MODULE_LIST
    #
    MatchObject = ReFindFirstWordRe.match(Comment)
    if not (MatchObject and MatchObject.group(1) in SUP_MODULE_LIST):
        return None, Comment

    #
    # parse line, it must have supported module type specified
    #
    if Comment.find(TAB_COMMENT_SPLIT) == -1:
        Comment += TAB_COMMENT_SPLIT    
    SupMode, HelpStr = GetSplitValueList(Comment, TAB_COMMENT_SPLIT, 1)
    SupModuleList = []
    for Mod in GetSplitValueList(SupMode, TAB_SPACE_SPLIT):
        if not Mod:
            continue
        elif Mod not in SUP_MODULE_LIST:
            Logger.Error("UPT",
                         FORMAT_INVALID,
                         ST.WRN_INVALID_MODULE_TYPE%Mod, 
                         ContainerFile, 
                         LineNum)
        else:
            SupModuleList.append(Mod)

    return SupModuleList, HelpStr

## _CheckListExpression
#
# @param Expression: Pcd value list expression 
#
def _CheckListExpression(Expression):
    ListExpr = ''
    if TAB_VALUE_SPLIT in Expression:
        ListExpr = Expression[Expression.find(TAB_VALUE_SPLIT)+1:] 
    else:
        ListExpr = Expression
        
    return IsValidListExpr(ListExpr)

## _CheckExpreesion
#
# @param Expression: Pcd value expression
#
def _CheckExpression(Expression):
    Expr = ''
    if TAB_VALUE_SPLIT in Expression:
        Expr = Expression[Expression.find(TAB_VALUE_SPLIT)+1:]
    else:
        Expr = Expression
    return IsValidLogicalExpr(Expr, True)

## _CheckRangeExpression
#
# @param Expression:    Pcd range expression
#          
def _CheckRangeExpression(Expression):
    RangeExpr = ''
    if TAB_VALUE_SPLIT in Expression:
        RangeExpr = Expression[Expression.find(TAB_VALUE_SPLIT)+1:]
    else:
        RangeExpr = Expression
        
    return IsValidRangeExpr(RangeExpr)

## ValidateCopyright
#
#
#
def ValidateCopyright(Result, ErrType, FileName, LineNo, ErrMsg):
    if not Result:
        Logger.Warn("\nUPT", ErrType, FileName, LineNo, ErrMsg) 

## _ValidateCopyright
#
# @param Line:    Line that contains copyright information, # stripped
# 
# @retval Result: True if line is conformed to Spec format, False else
# @retval ErrMsg: the detailed error description
#  
def _ValidateCopyright(Line):
    if Line:
        pass
    Result = True
    ErrMsg = ''
    
    return Result, ErrMsg

def GenerateTokenList (Comment):
    #
    # Tokenize Comment using '#' and ' ' as token seperators
    #
    RelplacedComment = None    
    while Comment != RelplacedComment:
        RelplacedComment = Comment
        Comment = Comment.replace('##', '#').replace('  ', ' ').replace(' ', '#').strip('# ')
    return Comment.split('#')


#
# Comment       - Comment to parse
# TypeTokens    - A dictionary of type token synonyms
# RemoveTokens  - A list of tokens to remove from help text
# ParseVariable - True for parsing [Guids].  Otherwise False
#
def ParseComment (Comment, UsageTokens, TypeTokens, RemoveTokens, ParseVariable):
    #
    # Initialize return values
    #
    Usage = None
    Type = None
    String = None
    
    Comment = Comment[0]
    
    NumTokens = 2  
    if ParseVariable:
        # 
        # Remove white space around first instance of ':' from Comment if 'Variable' 
        # is in front of ':' and Variable is the 1st or 2nd token in Comment.
        #
        List = Comment.split(':', 1)    
        if len(List) > 1:
            SubList = GenerateTokenList (List[0].strip())
            if len(SubList) in [1, 2] and SubList[-1] == 'Variable':
                if List[1].strip().find('L"') == 0:      
                    Comment = List[0].strip() + ':' + List[1].strip()
        
        # 
        # Remove first instance of L"<VariableName> from Comment and put into String
        # if and only if L"<VariableName>" is the 1st token, the 2nd token.  Or 
        # L"<VariableName>" is the third token immediately following 'Variable:'.
        #
        End = -1
        Start = Comment.find('Variable:L"')
        if Start >= 0:
            String = Comment[Start + 9:]
            End = String[2:].find('"')
        else:
            Start = Comment.find('L"')
            if Start >= 0:
                String = Comment[Start:]
                End = String[2:].find('"')
        if End >= 0:
            SubList = GenerateTokenList (Comment[:Start])
            if len(SubList) < 2: 
                Comment = Comment[:Start] + String[End + 3:]
                String = String[:End + 3]
                Type = 'Variable'
                NumTokens = 1  
    
    #
    # Initialze HelpText to Comment.  
    # Content will be remove from HelpText as matching tokens are found
    #  
    HelpText = Comment
    
    #
    # Tokenize Comment using '#' and ' ' as token seperators
    #
    List = GenerateTokenList (Comment)
    
    #
    # Search first two tokens for Usage and Type and remove any matching tokens 
    # from HelpText
    #
    for Token in List[0:NumTokens]:
        if Usage == None and Token in UsageTokens:
            Usage = UsageTokens[Token]
            HelpText = HelpText.replace(Token, '')
    if Usage != None or not ParseVariable:
        for Token in List[0:NumTokens]:
            if Type == None and Token in TypeTokens:
                Type = TypeTokens[Token]
                HelpText = HelpText.replace(Token, '')
            if Usage != None:    
                for Token in List[0:NumTokens]:
                    if Token in RemoveTokens:
                        HelpText = HelpText.replace(Token, '')
    
    #
    # If no Usage token is present and set Usage to UNDEFINED
    #  
    if Usage == None:
        Usage = 'UNDEFINED'
    
    #
    # If no Type token is present and set Type to UNDEFINED
    #  
    if Type == None:
        Type = 'UNDEFINED'
    
    #
    # If Type is not 'Variable:', then set String to None
    #  
    if Type != 'Variable':
        String = None  
    
    #
    # Strip ' ' and '#' from the beginning of HelpText
    # If HelpText is an empty string after all parsing is 
    # complete then set HelpText to None
    #  
    HelpText = HelpText.lstrip('# ')
    if HelpText == '':
        HelpText = None
      
    #
    # Return parsing results
    #  
    return Usage, Type, String, HelpText  
