## @file
# preprocess source file
#
#  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import re
import Common.LongFilePathOs as os
import sys

import antlr3
from CLexer import CLexer
from CParser import CParser

import FileProfile
from CodeFragment import PP_Directive
from ParserWarning import Warning


##define T_CHAR_SPACE                ' '
##define T_CHAR_NULL                 '\0'
##define T_CHAR_CR                   '\r'
##define T_CHAR_TAB                  '\t'
##define T_CHAR_LF                   '\n'
##define T_CHAR_SLASH                '/'
##define T_CHAR_BACKSLASH            '\\'
##define T_CHAR_DOUBLE_QUOTE         '\"'
##define T_CHAR_SINGLE_QUOTE         '\''
##define T_CHAR_STAR                 '*'
##define T_CHAR_HASH                 '#'

(T_CHAR_SPACE, T_CHAR_NULL, T_CHAR_CR, T_CHAR_TAB, T_CHAR_LF, T_CHAR_SLASH, \
T_CHAR_BACKSLASH, T_CHAR_DOUBLE_QUOTE, T_CHAR_SINGLE_QUOTE, T_CHAR_STAR, T_CHAR_HASH) = \
(' ', '\0', '\r', '\t', '\n', '/', '\\', '\"', '\'', '*', '#')

SEPERATOR_TUPLE = ('=', '|', ',', '{', '}')

(T_COMMENT_TWO_SLASH, T_COMMENT_SLASH_STAR) = (0, 1)

(T_PP_INCLUDE, T_PP_DEFINE, T_PP_OTHERS) = (0, 1, 2)

## The collector for source code fragments.
#
# PreprocessFile method should be called prior to ParseFile
#
# GetNext*** procedures mean these procedures will get next token first, then make judgement.
# Get*** procedures mean these procedures will make judgement on current token only.
#
class CodeFragmentCollector:
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.Profile = FileProfile.FileProfile(FileName)
        self.Profile.FileLinesList.append(T_CHAR_LF)
        self.FileName = FileName
        self.CurrentLineNumber = 1
        self.CurrentOffsetWithinLine = 0

        self.__Token = ""
        self.__SkippedChars = ""

    ## __IsWhiteSpace() method
    #
    #   Whether char at current FileBufferPos is whitespace
    #
    #   @param  self        The object pointer
    #   @param  Char        The char to test
    #   @retval True        The char is a kind of white space
    #   @retval False       The char is NOT a kind of white space
    #
    def __IsWhiteSpace(self, Char):
        if Char in (T_CHAR_NULL, T_CHAR_CR, T_CHAR_SPACE, T_CHAR_TAB, T_CHAR_LF):
            return True
        else:
            return False

    ## __SkipWhiteSpace() method
    #
    #   Skip white spaces from current char, return number of chars skipped
    #
    #   @param  self        The object pointer
    #   @retval Count       The number of chars skipped
    #
    def __SkipWhiteSpace(self):
        Count = 0
        while not self.__EndOfFile():
            Count += 1
            if self.__CurrentChar() in (T_CHAR_NULL, T_CHAR_CR, T_CHAR_LF, T_CHAR_SPACE, T_CHAR_TAB):
                self.__SkippedChars += str(self.__CurrentChar())
                self.__GetOneChar()

            else:
                Count = Count - 1
                return Count

    ## __EndOfFile() method
    #
    #   Judge current buffer pos is at file end
    #
    #   @param  self        The object pointer
    #   @retval True        Current File buffer position is at file end
    #   @retval False       Current File buffer position is NOT at file end
    #
    def __EndOfFile(self):
        NumberOfLines = len(self.Profile.FileLinesList)
        SizeOfLastLine = len(self.Profile.FileLinesList[-1])
        if self.CurrentLineNumber == NumberOfLines and self.CurrentOffsetWithinLine >= SizeOfLastLine - 1:
            return True
        elif self.CurrentLineNumber > NumberOfLines:
            return True
        else:
            return False

    ## __EndOfLine() method
    #
    #   Judge current buffer pos is at line end
    #
    #   @param  self        The object pointer
    #   @retval True        Current File buffer position is at line end
    #   @retval False       Current File buffer position is NOT at line end
    #
    def __EndOfLine(self):
        SizeOfCurrentLine = len(self.Profile.FileLinesList[self.CurrentLineNumber - 1])
        if self.CurrentOffsetWithinLine >= SizeOfCurrentLine - 1:
            return True
        else:
            return False

    ## Rewind() method
    #
    #   Reset file data buffer to the initial state
    #
    #   @param  self        The object pointer
    #
    def Rewind(self):
        self.CurrentLineNumber = 1
        self.CurrentOffsetWithinLine = 0

    ## __UndoOneChar() method
    #
    #   Go back one char in the file buffer
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully go back one char
    #   @retval False       Not able to go back one char as file beginning reached
    #
    def __UndoOneChar(self):

        if self.CurrentLineNumber == 1 and self.CurrentOffsetWithinLine == 0:
            return False
        elif self.CurrentOffsetWithinLine == 0:
            self.CurrentLineNumber -= 1
            self.CurrentOffsetWithinLine = len(self.__CurrentLine()) - 1
        else:
            self.CurrentOffsetWithinLine -= 1
        return True

    ## __GetOneChar() method
    #
    #   Move forward one char in the file buffer
    #
    #   @param  self        The object pointer
    #
    def __GetOneChar(self):
        if self.CurrentOffsetWithinLine == len(self.Profile.FileLinesList[self.CurrentLineNumber - 1]) - 1:
                self.CurrentLineNumber += 1
                self.CurrentOffsetWithinLine = 0
        else:
                self.CurrentOffsetWithinLine += 1

    ## __CurrentChar() method
    #
    #   Get the char pointed to by the file buffer pointer
    #
    #   @param  self        The object pointer
    #   @retval Char        Current char
    #
    def __CurrentChar(self):
        CurrentChar = self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine]

        return CurrentChar

    ## __NextChar() method
    #
    #   Get the one char pass the char pointed to by the file buffer pointer
    #
    #   @param  self        The object pointer
    #   @retval Char        Next char
    #
    def __NextChar(self):
        if self.CurrentOffsetWithinLine == len(self.Profile.FileLinesList[self.CurrentLineNumber - 1]) - 1:
            return self.Profile.FileLinesList[self.CurrentLineNumber][0]
        else:
            return self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine + 1]

    ## __SetCurrentCharValue() method
    #
    #   Modify the value of current char
    #
    #   @param  self        The object pointer
    #   @param  Value       The new value of current char
    #
    def __SetCurrentCharValue(self, Value):
        self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine] = Value

    ## __SetCharValue() method
    #
    #   Modify the value of current char
    #
    #   @param  self        The object pointer
    #   @param  Value       The new value of current char
    #
    def __SetCharValue(self, Line, Offset, Value):
        self.Profile.FileLinesList[Line - 1][Offset] = Value

    ## __CurrentLine() method
    #
    #   Get the list that contains current line contents
    #
    #   @param  self        The object pointer
    #   @retval List        current line contents
    #
    def __CurrentLine(self):
        return self.Profile.FileLinesList[self.CurrentLineNumber - 1]

    ## __InsertComma() method
    #
    #   Insert ',' to replace PP
    #
    #   @param  self        The object pointer
    #   @retval List        current line contents
    #
    def __InsertComma(self, Line):


        if self.Profile.FileLinesList[Line - 1][0] != T_CHAR_HASH:
            BeforeHashPart = str(self.Profile.FileLinesList[Line - 1]).split(T_CHAR_HASH)[0]
            if BeforeHashPart.rstrip().endswith(T_CHAR_COMMA) or BeforeHashPart.rstrip().endswith(';'):
                return

        if Line - 2 >= 0 and str(self.Profile.FileLinesList[Line - 2]).rstrip().endswith(','):
            return

        if Line - 2 >= 0 and str(self.Profile.FileLinesList[Line - 2]).rstrip().endswith(';'):
            return

        if str(self.Profile.FileLinesList[Line]).lstrip().startswith(',') or str(self.Profile.FileLinesList[Line]).lstrip().startswith(';'):
            return

        self.Profile.FileLinesList[Line - 1].insert(self.CurrentOffsetWithinLine, ',')

    ## PreprocessFileWithClear() method
    #
    # Run a preprocess for the file to clean all comments
    #
    # @param  self        The object pointer
    #
    def PreprocessFileWithClear(self):

        self.Rewind()
        InComment = False
        DoubleSlashComment = False
        HashComment = False
        PPExtend = False
        PPDirectiveObj = None
        # HashComment in quoted string " " is ignored.
        InString = False
        InCharLiteral = False

        self.Profile.FileLinesList = [list(s) for s in self.Profile.FileLinesListFromFile]
        while not self.__EndOfFile():

            if not InComment and self.__CurrentChar() == T_CHAR_DOUBLE_QUOTE:
                InString = not InString

            if not InComment and self.__CurrentChar() == T_CHAR_SINGLE_QUOTE:
                InCharLiteral = not InCharLiteral
            # meet new line, then no longer in a comment for // and '#'
            if self.__CurrentChar() == T_CHAR_LF:
                if HashComment and PPDirectiveObj != None:
                    if PPDirectiveObj.Content.rstrip(T_CHAR_CR).endswith(T_CHAR_BACKSLASH):
                        PPDirectiveObj.Content += T_CHAR_LF
                        PPExtend = True
                    else:
                        PPExtend = False

                EndLinePos = (self.CurrentLineNumber, self.CurrentOffsetWithinLine)

                if InComment and DoubleSlashComment:
                    InComment = False
                    DoubleSlashComment = False

                if InComment and HashComment and not PPExtend:
                    InComment = False
                    HashComment = False
                    PPDirectiveObj.Content += T_CHAR_LF
                    PPDirectiveObj.EndPos = EndLinePos
                    FileProfile.PPDirectiveList.append(PPDirectiveObj)
                    PPDirectiveObj = None

                if InString or InCharLiteral:
                    CurrentLine = "".join(self.__CurrentLine())
                    if CurrentLine.rstrip(T_CHAR_LF).rstrip(T_CHAR_CR).endswith(T_CHAR_BACKSLASH):
                        SlashIndex = CurrentLine.rindex(T_CHAR_BACKSLASH)
                        self.__SetCharValue(self.CurrentLineNumber, SlashIndex, T_CHAR_SPACE)

                self.CurrentLineNumber += 1
                self.CurrentOffsetWithinLine = 0
            # check for */ comment end
            elif InComment and not DoubleSlashComment and not HashComment and self.__CurrentChar() == T_CHAR_STAR and self.__NextChar() == T_CHAR_SLASH:

                self.__SetCurrentCharValue(T_CHAR_SPACE)
                self.__GetOneChar()
                self.__SetCurrentCharValue(T_CHAR_SPACE)
                self.__GetOneChar()
                InComment = False
            # set comments to spaces
            elif InComment:
                if HashComment:
                    # // follows hash PP directive
                    if self.__CurrentChar() == T_CHAR_SLASH and self.__NextChar() == T_CHAR_SLASH:
                        InComment = False
                        HashComment = False
                        PPDirectiveObj.EndPos = (self.CurrentLineNumber, self.CurrentOffsetWithinLine - 1)
                        FileProfile.PPDirectiveList.append(PPDirectiveObj)
                        PPDirectiveObj = None
                        continue
                    else:
                        PPDirectiveObj.Content += self.__CurrentChar()

                self.__SetCurrentCharValue(T_CHAR_SPACE)
                self.__GetOneChar()
            # check for // comment
            elif self.__CurrentChar() == T_CHAR_SLASH and self.__NextChar() == T_CHAR_SLASH:
                InComment = True
                DoubleSlashComment = True

            # check for '#' comment
            elif self.__CurrentChar() == T_CHAR_HASH and not InString and not InCharLiteral:
                InComment = True
                HashComment = True
                PPDirectiveObj = PP_Directive('', (self.CurrentLineNumber, self.CurrentOffsetWithinLine), None)
            # check for /* comment start
            elif self.__CurrentChar() == T_CHAR_SLASH and self.__NextChar() == T_CHAR_STAR:

                self.__SetCurrentCharValue( T_CHAR_SPACE)
                self.__GetOneChar()
                self.__SetCurrentCharValue( T_CHAR_SPACE)
                self.__GetOneChar()
                InComment = True
            else:
                self.__GetOneChar()

        EndLinePos = (self.CurrentLineNumber, self.CurrentOffsetWithinLine)

        if InComment and HashComment and not PPExtend:
            PPDirectiveObj.EndPos = EndLinePos
            FileProfile.PPDirectiveList.append(PPDirectiveObj)
        self.Rewind()

    ## ParseFile() method
    #
    #   Parse the file profile buffer to extract fd, fv ... information
    #   Exception will be raised if syntax error found
    #
    #   @param  self        The object pointer
    #
    def ParseFile(self):
        self.PreprocessFileWithClear()
        # restore from ListOfList to ListOfString
        self.Profile.FileLinesList = ["".join(list) for list in self.Profile.FileLinesList]
        FileStringContents = ''
        for fileLine in self.Profile.FileLinesList:
            FileStringContents += fileLine
        cStream = antlr3.StringStream(FileStringContents)
        lexer = CLexer(cStream)
        tStream = antlr3.CommonTokenStream(lexer)
        parser = CParser(tStream)
        parser.translation_unit()

    ## CleanFileProfileBuffer() method
    #
    #   Reset all contents of the profile of a file
    #
    def CleanFileProfileBuffer(self):

        FileProfile.PPDirectiveList = []
        FileProfile.AssignmentExpressionList = []
        FileProfile.FunctionDefinitionList = []
        FileProfile.VariableDeclarationList = []
        FileProfile.EnumerationDefinitionList = []
        FileProfile.StructUnionDefinitionList = []
        FileProfile.TypedefDefinitionList = []
        FileProfile.FunctionCallingList = []

    ## PrintFragments() method
    #
    #   Print the contents of the profile of a file
    #
    def PrintFragments(self):

        print '################# ' + self.FileName + '#####################'

        print '/****************************************/'
        print '/*************** ASSIGNMENTS ***************/'
        print '/****************************************/'
        for asign in FileProfile.AssignmentExpressionList:
            print str(asign.StartPos) + asign.Name + asign.Operator + asign.Value

        print '/****************************************/'
        print '/********* PREPROCESS DIRECTIVES ********/'
        print '/****************************************/'
        for pp in FileProfile.PPDirectiveList:
            print str(pp.StartPos) + pp.Content

        print '/****************************************/'
        print '/********* VARIABLE DECLARATIONS ********/'
        print '/****************************************/'
        for var in FileProfile.VariableDeclarationList:
            print str(var.StartPos) + var.Modifier + ' '+ var.Declarator

        print '/****************************************/'
        print '/********* FUNCTION DEFINITIONS *********/'
        print '/****************************************/'
        for func in FileProfile.FunctionDefinitionList:
            print str(func.StartPos) + func.Modifier + ' '+ func.Declarator + ' ' + str(func.NamePos)

        print '/****************************************/'
        print '/************ ENUMERATIONS **************/'
        print '/****************************************/'
        for enum in FileProfile.EnumerationDefinitionList:
            print str(enum.StartPos) + enum.Content

        print '/****************************************/'
        print '/*********** STRUCTS/UNIONS *************/'
        print '/****************************************/'
        for su in FileProfile.StructUnionDefinitionList:
            print str(su.StartPos) + su.Content

        print '/****************************************/'
        print '/************** TYPEDEFS ****************/'
        print '/****************************************/'
        for typedef in FileProfile.TypedefDefinitionList:
            print str(typedef.StartPos) + typedef.ToType

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == "__main__":

    print "For Test."
