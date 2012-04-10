## @file
# This file is used to define common string related functions used in parsing process
#
# Copyright (c) 2007 - 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import re
import DataType
import os.path
import string
import EdkLogger as EdkLogger

import GlobalData
from BuildToolError import *
from CommonDataClass.Exceptions import *

gHexVerPatt = re.compile('0x[a-f0-9]{4}[a-f0-9]{4}$', re.IGNORECASE)
gHumanReadableVerPatt = re.compile(r'([1-9][0-9]*|0)\.[0-9]{1,2}$')

## GetSplitValueList
#
# Get a value list from a string with multiple values splited with SplitTag
# The default SplitTag is DataType.TAB_VALUE_SPLIT
# 'AAA|BBB|CCC' -> ['AAA', 'BBB', 'CCC']
#
# @param String:    The input string to be splitted
# @param SplitTag:  The split key, default is DataType.TAB_VALUE_SPLIT
# @param MaxSplit:  The max number of split values, default is -1
#
# @retval list() A list for splitted string
#
def GetSplitValueList(String, SplitTag=DataType.TAB_VALUE_SPLIT, MaxSplit= -1):
    ValueList = []
    Last = 0
    Escaped = False
    InString = False
    for Index in range(0, len(String)):
        Char = String[Index]

        if not Escaped:
            # Found a splitter not in a string, split it
            if not InString and Char == SplitTag:
                ValueList.append(String[Last:Index].strip())
                Last = Index + 1
                if MaxSplit > 0 and len(ValueList) >= MaxSplit:
                    break

            if Char == '\\' and InString:
                Escaped = True
            elif Char == '"':
                if not InString:
                    InString = True
                else:
                    InString = False
        else:
            Escaped = False

    if Last < len(String):
        ValueList.append(String[Last:].strip())
    elif Last == len(String):
        ValueList.append('')

    return ValueList

## GetSplitList
#
# Get a value list from a string with multiple values splited with SplitString
# The default SplitTag is DataType.TAB_VALUE_SPLIT
# 'AAA|BBB|CCC' -> ['AAA', 'BBB', 'CCC']
#
# @param String:    The input string to be splitted
# @param SplitStr:  The split key, default is DataType.TAB_VALUE_SPLIT
# @param MaxSplit:  The max number of split values, default is -1
#
# @retval list() A list for splitted string
#
def GetSplitList(String, SplitStr=DataType.TAB_VALUE_SPLIT, MaxSplit= -1):
    return map(lambda l: l.strip(), String.split(SplitStr, MaxSplit))

## MergeArches
#
# Find a key's all arches in dict, add the new arch to the list
# If not exist any arch, set the arch directly
#
# @param Dict:  The input value for Dict
# @param Key:   The input value for Key
# @param Arch:  The Arch to be added or merged
#
def MergeArches(Dict, Key, Arch):
    if Key in Dict.keys():
        Dict[Key].append(Arch)
    else:
        Dict[Key] = Arch.split()

## GenDefines
#
# Parse a string with format "DEFINE <VarName> = <PATH>"
# Generate a map Defines[VarName] = PATH
# Return False if invalid format
#
# @param String:   String with DEFINE statement
# @param Arch:     Supportted Arch
# @param Defines:  DEFINE statement to be parsed
#
# @retval 0   DEFINE statement found, and valid
# @retval 1   DEFINE statement found, but not valid
# @retval -1  DEFINE statement not found
#
def GenDefines(String, Arch, Defines):
    if String.find(DataType.TAB_DEFINE + ' ') > -1:
        List = String.replace(DataType.TAB_DEFINE + ' ', '').split(DataType.TAB_EQUAL_SPLIT)
        if len(List) == 2:
            Defines[(CleanString(List[0]), Arch)] = CleanString(List[1])
            return 0
        else:
            return -1

    return 1

## GenInclude
#
# Parse a string with format "!include <Filename>"
# Return the file path
# Return False if invalid format or NOT FOUND
#
# @param String:        String with INCLUDE statement
# @param IncludeFiles:  INCLUDE statement to be parsed
# @param Arch:          Supportted Arch
#
# @retval True
# @retval False
#
def GenInclude(String, IncludeFiles, Arch):
    if String.upper().find(DataType.TAB_INCLUDE.upper() + ' ') > -1:
        IncludeFile = CleanString(String[String.upper().find(DataType.TAB_INCLUDE.upper() + ' ') + len(DataType.TAB_INCLUDE + ' ') : ])
        MergeArches(IncludeFiles, IncludeFile, Arch)
        return True
    else:
        return False

## GetLibraryClassesWithModuleType
#
# Get Library Class definition when no module type defined
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval True Get library classes successfully
#
def GetLibraryClassesWithModuleType(Lines, Key, KeyValues, CommentCharacter):
    newKey = SplitModuleType(Key)
    Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    LineList = Lines.splitlines()
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line != '' and Line[0] != CommentCharacter:
            KeyValues.append([CleanString(Line, CommentCharacter), newKey[1]])

    return True

## GetDynamics
#
# Get Dynamic Pcds
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval True Get Dynamic Pcds successfully
#
def GetDynamics(Lines, Key, KeyValues, CommentCharacter):
    #
    # Get SkuId Name List
    #
    SkuIdNameList = SplitModuleType(Key)

    Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    LineList = Lines.splitlines()
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line != '' and Line[0] != CommentCharacter:
            KeyValues.append([CleanString(Line, CommentCharacter), SkuIdNameList[1]])

    return True

## SplitModuleType
#
# Split ModuleType out of section defien to get key
# [LibraryClass.Arch.ModuleType|ModuleType|ModuleType] -> [ 'LibraryClass.Arch', ['ModuleType', 'ModuleType', 'ModuleType'] ]
#
# @param Key:  String to be parsed
#
# @retval ReturnValue A list for module types
#
def SplitModuleType(Key):
    KeyList = Key.split(DataType.TAB_SPLIT)
    #
    # Fill in for arch
    #
    KeyList.append('')
    #
    # Fill in for moduletype
    #
    KeyList.append('')
    ReturnValue = []
    KeyValue = KeyList[0]
    if KeyList[1] != '':
        KeyValue = KeyValue + DataType.TAB_SPLIT + KeyList[1]
    ReturnValue.append(KeyValue)
    ReturnValue.append(GetSplitValueList(KeyList[2]))

    return ReturnValue

## Replace macro in strings list
#
# This method replace macros used in a given string list. The macros are
# given in a dictionary.
#
# @param StringList         StringList to be processed
# @param MacroDefinitions   The macro definitions in the form of dictionary
# @param SelfReplacement    To decide whether replace un-defined macro to ''
#
# @retval NewList           A new string list whose macros are replaced
#
def ReplaceMacros(StringList, MacroDefinitions={}, SelfReplacement=False):
    NewList = []
    for String in StringList:
        if type(String) == type(''):
            NewList.append(ReplaceMacro(String, MacroDefinitions, SelfReplacement))
        else:
            NewList.append(String)

    return NewList

## Replace macro in string
#
# This method replace macros used in given string. The macros are given in a
# dictionary.
#
# @param String             String to be processed
# @param MacroDefinitions   The macro definitions in the form of dictionary
# @param SelfReplacement    To decide whether replace un-defined macro to ''
#
# @retval string            The string whose macros are replaced
#
def ReplaceMacro(String, MacroDefinitions={}, SelfReplacement=False, RaiseError=False):
    LastString = String
    while String and MacroDefinitions:
        MacroUsed = GlobalData.gMacroRefPattern.findall(String)
        # no macro found in String, stop replacing
        if len(MacroUsed) == 0:
            break

        for Macro in MacroUsed:
            if Macro not in MacroDefinitions:
                if RaiseError:
                    raise SymbolNotFound("%s not defined" % Macro)
                if SelfReplacement:
                    String = String.replace("$(%s)" % Macro, '')
                continue
            String = String.replace("$(%s)" % Macro, MacroDefinitions[Macro])
        # in case there's macro not defined
        if String == LastString:
            break
        LastString = String

    return String

## NormPath
#
# Create a normal path
# And replace DFEINE in the path
#
# @param Path:     The input value for Path to be converted
# @param Defines:  A set for DEFINE statement
#
# @retval Path Formatted path
#
def NormPath(Path, Defines={}):
    IsRelativePath = False
    if Path:
        if Path[0] == '.':
            IsRelativePath = True
        #
        # Replace with Define
        #
        if Defines:
            Path = ReplaceMacro(Path, Defines)
        #
        # To local path format
        #
        Path = os.path.normpath(Path)

    if IsRelativePath and Path[0] != '.':
        Path = os.path.join('.', Path)

    return Path

## CleanString
#
# Remove comments in a string
# Remove spaces
#
# @param Line:              The string to be cleaned
# @param CommentCharacter:  Comment char, used to ignore comment content, default is DataType.TAB_COMMENT_SPLIT
#
# @retval Path Formatted path
#
def CleanString(Line, CommentCharacter=DataType.TAB_COMMENT_SPLIT, AllowCppStyleComment=False, BuildOption=False):
    #
    # remove whitespace
    #
    Line = Line.strip();
    #
    # Replace Edk's comment character
    #
    if AllowCppStyleComment:
        Line = Line.replace(DataType.TAB_COMMENT_EDK_SPLIT, CommentCharacter)
    #
    # remove comments, but we should escape comment character in string
    #
    InString = False
    CommentInString = False
    for Index in range(0, len(Line)):
        if Line[Index] == '"':
            InString = not InString
        elif Line[Index] == CommentCharacter and InString :
            CommentInString = True
        elif Line[Index] == CommentCharacter and not InString :
            Line = Line[0: Index]
            break

    if CommentInString and BuildOption:
        Line = Line.replace('"', '')
        ChIndex = Line.find('#')
        while ChIndex >= 0:
            if GlobalData.gIsWindows:
                if ChIndex == 0 or Line[ChIndex - 1] != '^':
                    Line = Line[0:ChIndex] + '^' + Line[ChIndex:]
                    ChIndex = Line.find('#', ChIndex + 2)
                else:
                    ChIndex = Line.find('#', ChIndex + 1)
            else:
                if ChIndex == 0 or Line[ChIndex - 1] != '\\':
                    Line = Line[0:ChIndex] + '\\' + Line[ChIndex:]
                    ChIndex = Line.find('#', ChIndex + 2)
                else:
                    ChIndex = Line.find('#', ChIndex + 1)
    #
    # remove whitespace again
    #
    Line = Line.strip();

    return Line

## CleanString2
#
# Split comments in a string
# Remove spaces
#
# @param Line:              The string to be cleaned
# @param CommentCharacter:  Comment char, used to ignore comment content, default is DataType.TAB_COMMENT_SPLIT
#
# @retval Path Formatted path
#
def CleanString2(Line, CommentCharacter=DataType.TAB_COMMENT_SPLIT, AllowCppStyleComment=False):
    #
    # remove whitespace
    #
    Line = Line.strip();
    #
    # Replace Edk's comment character
    #
    if AllowCppStyleComment:
        Line = Line.replace(DataType.TAB_COMMENT_EDK_SPLIT, CommentCharacter)
    #
    # separate comments and statements
    #
    LineParts = Line.split(CommentCharacter, 1);
    #
    # remove whitespace again
    #
    Line = LineParts[0].strip();
    if len(LineParts) > 1:
        Comment = LineParts[1].strip()
        # Remove prefixed and trailing comment characters
        Start = 0
        End = len(Comment)
        while Start < End and Comment.startswith(CommentCharacter, Start, End):
            Start += 1
        while End >= 0 and Comment.endswith(CommentCharacter, Start, End):
            End -= 1
        Comment = Comment[Start:End]
        Comment = Comment.strip()
    else:
        Comment = ''

    return Line, Comment

## GetMultipleValuesOfKeyFromLines
#
# Parse multiple strings to clean comment and spaces
# The result is saved to KeyValues
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval True Successfully executed
#
def GetMultipleValuesOfKeyFromLines(Lines, Key, KeyValues, CommentCharacter):
    Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    LineList = Lines.split('\n')
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line != '' and Line[0] != CommentCharacter:
            KeyValues += [Line]

    return True

## GetDefineValue
#
# Parse a DEFINE statement to get defined value
# DEFINE Key Value
#
# @param String:            The content to be parsed
# @param Key:               The key of DEFINE statement
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval string The defined value
#
def GetDefineValue(String, Key, CommentCharacter):
    String = CleanString(String)
    return String[String.find(Key + ' ') + len(Key + ' ') : ]

## GetHexVerValue
#
# Get a Hex Version Value
#
# @param VerString:         The version string to be parsed
#
#
# @retval:      If VerString is incorrectly formatted, return "None" which will break the build.
#               If VerString is correctly formatted, return a Hex value of the Version Number (0xmmmmnnnn)
#                   where mmmm is the major number and nnnn is the adjusted minor number.
#
def GetHexVerValue(VerString):
    VerString = CleanString(VerString)

    if gHumanReadableVerPatt.match(VerString):
        ValueList = VerString.split('.')
        Major = ValueList[0]
        Minor = ValueList[1]
        if len(Minor) == 1:
            Minor += '0'
        DeciValue = (int(Major) << 16) + int(Minor);
        return "0x%08x" % DeciValue
    elif gHexVerPatt.match(VerString):
        return VerString
    else:
        return None


## GetSingleValueOfKeyFromLines
#
# Parse multiple strings as below to get value of each definition line
# Key1 = Value1
# Key2 = Value2
# The result is saved to Dictionary
#
# @param Lines:                The content to be parsed
# @param Dictionary:           To store data after parsing
# @param CommentCharacter:     Comment char, be used to ignore comment content
# @param KeySplitCharacter:    Key split char, between key name and key value. Key1 = Value1, '=' is the key split char
# @param ValueSplitFlag:       Value split flag, be used to decide if has multiple values
# @param ValueSplitCharacter:  Value split char, be used to split multiple values. Key1 = Value1|Value2, '|' is the value split char
#
# @retval True Successfully executed
#
def GetSingleValueOfKeyFromLines(Lines, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
    Lines = Lines.split('\n')
    Keys = []
    Value = ''
    DefineValues = ['']
    SpecValues = ['']

    for Line in Lines:
        #
        # Handle DEFINE and SPEC
        #
        if Line.find(DataType.TAB_INF_DEFINES_DEFINE + ' ') > -1:
            if '' in DefineValues:
                DefineValues.remove('')
            DefineValues.append(GetDefineValue(Line, DataType.TAB_INF_DEFINES_DEFINE, CommentCharacter))
            continue
        if Line.find(DataType.TAB_INF_DEFINES_SPEC + ' ') > -1:
            if '' in SpecValues:
                SpecValues.remove('')
            SpecValues.append(GetDefineValue(Line, DataType.TAB_INF_DEFINES_SPEC, CommentCharacter))
            continue

        #
        # Handle Others
        #
        LineList = Line.split(KeySplitCharacter, 1)
        if len(LineList) >= 2:
            Key = LineList[0].split()
            if len(Key) == 1 and Key[0][0] != CommentCharacter:
                #
                # Remove comments and white spaces
                #
                LineList[1] = CleanString(LineList[1], CommentCharacter)
                if ValueSplitFlag:
                    Value = map(string.strip, LineList[1].split(ValueSplitCharacter))
                else:
                    Value = CleanString(LineList[1], CommentCharacter).splitlines()

                if Key[0] in Dictionary:
                    if Key[0] not in Keys:
                        Dictionary[Key[0]] = Value
                        Keys.append(Key[0])
                    else:
                        Dictionary[Key[0]].extend(Value)
                else:
                    Dictionary[DataType.TAB_INF_DEFINES_MACRO][Key[0]] = Value[0]

    if DefineValues == []:
        DefineValues = ['']
    if SpecValues == []:
        SpecValues = ['']
    Dictionary[DataType.TAB_INF_DEFINES_DEFINE] = DefineValues
    Dictionary[DataType.TAB_INF_DEFINES_SPEC] = SpecValues

    return True

## The content to be parsed
#
# Do pre-check for a file before it is parsed
# Check $()
# Check []
#
# @param FileName:       Used for error report
# @param FileContent:    File content to be parsed
# @param SupSectionTag:  Used for error report
#
def PreCheck(FileName, FileContent, SupSectionTag):
    LineNo = 0
    IsFailed = False
    NewFileContent = ''
    for Line in FileContent.splitlines():
        LineNo = LineNo + 1
        #
        # Clean current line
        #
        Line = CleanString(Line)

        #
        # Remove commented line
        #
        if Line.find(DataType.TAB_COMMA_SPLIT) == 0:
            Line = ''
        #
        # Check $()
        #
        if Line.find('$') > -1:
            if Line.find('$(') < 0 or Line.find(')') < 0:
                EdkLogger.error("Parser", FORMAT_INVALID, Line=LineNo, File=FileName, RaiseError=EdkLogger.IsRaiseError)

        #
        # Check []
        #
        if Line.find('[') > -1 or Line.find(']') > -1:
            #
            # Only get one '[' or one ']'
            #
            if not (Line.find('[') > -1 and Line.find(']') > -1):
                EdkLogger.error("Parser", FORMAT_INVALID, Line=LineNo, File=FileName, RaiseError=EdkLogger.IsRaiseError)

        #
        # Regenerate FileContent
        #
        NewFileContent = NewFileContent + Line + '\r\n'

    if IsFailed:
       EdkLogger.error("Parser", FORMAT_INVALID, Line=LineNo, File=FileName, RaiseError=EdkLogger.IsRaiseError)

    return NewFileContent

## CheckFileType
#
# Check if the Filename is including ExtName
# Return True if it exists
# Raise a error message if it not exists
#
# @param CheckFilename:      Name of the file to be checked
# @param ExtName:            Ext name of the file to be checked
# @param ContainerFilename:  The container file which describes the file to be checked, used for error report
# @param SectionName:        Used for error report
# @param Line:               The line in container file which defines the file to be checked
#
# @retval True The file type is correct
#
def CheckFileType(CheckFilename, ExtName, ContainerFilename, SectionName, Line, LineNo= -1):
    if CheckFilename != '' and CheckFilename != None:
        (Root, Ext) = os.path.splitext(CheckFilename)
        if Ext.upper() != ExtName.upper():
            ContainerFile = open(ContainerFilename, 'r').read()
            if LineNo == -1:
                LineNo = GetLineNo(ContainerFile, Line)
            ErrorMsg = "Invalid %s. '%s' is found, but '%s' file is needed" % (SectionName, CheckFilename, ExtName)
            EdkLogger.error("Parser", PARSER_ERROR, ErrorMsg, Line=LineNo,
                            File=ContainerFilename, RaiseError=EdkLogger.IsRaiseError)

    return True

## CheckFileExist
#
# Check if the file exists
# Return True if it exists
# Raise a error message if it not exists
#
# @param CheckFilename:      Name of the file to be checked
# @param WorkspaceDir:       Current workspace dir
# @param ContainerFilename:  The container file which describes the file to be checked, used for error report
# @param SectionName:        Used for error report
# @param Line:               The line in container file which defines the file to be checked
#
# @retval The file full path if the file exists
#
def CheckFileExist(WorkspaceDir, CheckFilename, ContainerFilename, SectionName, Line, LineNo= -1):
    CheckFile = ''
    if CheckFilename != '' and CheckFilename != None:
        CheckFile = WorkspaceFile(WorkspaceDir, CheckFilename)
        if not os.path.isfile(CheckFile):
            ContainerFile = open(ContainerFilename, 'r').read()
            if LineNo == -1:
                LineNo = GetLineNo(ContainerFile, Line)
            ErrorMsg = "Can't find file '%s' defined in section '%s'" % (CheckFile, SectionName)
            EdkLogger.error("Parser", PARSER_ERROR, ErrorMsg,
                            File=ContainerFilename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)

    return CheckFile

## GetLineNo
#
# Find the index of a line in a file
#
# @param FileContent:  Search scope
# @param Line:         Search key
#
# @retval int  Index of the line
# @retval -1     The line is not found
#
def GetLineNo(FileContent, Line, IsIgnoreComment=True):
    LineList = FileContent.splitlines()
    for Index in range(len(LineList)):
        if LineList[Index].find(Line) > -1:
            #
            # Ignore statement in comment
            #
            if IsIgnoreComment:
                if LineList[Index].strip()[0] == DataType.TAB_COMMENT_SPLIT:
                    continue
            return Index + 1

    return -1

## RaiseParserError
#
# Raise a parser error
#
# @param Line:     String which has error
# @param Section:  Used for error report
# @param File:     File which has the string
# @param Format:   Correct format
#
def RaiseParserError(Line, Section, File, Format='', LineNo= -1):
    if LineNo == -1:
        LineNo = GetLineNo(open(os.path.normpath(File), 'r').read(), Line)
    ErrorMsg = "Invalid statement '%s' is found in section '%s'" % (Line, Section)
    if Format != '':
        Format = "Correct format is " + Format
    EdkLogger.error("Parser", PARSER_ERROR, ErrorMsg, File=File, Line=LineNo, ExtraData=Format, RaiseError=EdkLogger.IsRaiseError)

## WorkspaceFile
#
# Return a full path with workspace dir
#
# @param WorkspaceDir:  Workspace dir
# @param Filename:      Relative file name
#
# @retval string A full path
#
def WorkspaceFile(WorkspaceDir, Filename):
    return os.path.join(NormPath(WorkspaceDir), NormPath(Filename))

## Split string
#
# Revmove '"' which startswith and endswith string
#
# @param String:  The string need to be splited
#
# @retval String: The string after removed '""'
#
def SplitString(String):
    if String.startswith('\"'):
        String = String[1:]
    if String.endswith('\"'):
        String = String[:-1]

    return String

## Convert To Sql String
#
# 1. Replace "'" with "''" in each item of StringList
#
# @param StringList:  A list for strings to be converted
#
def ConvertToSqlString(StringList):
    return map(lambda s: s.replace("'", "''") , StringList)

## Convert To Sql String
#
# 1. Replace "'" with "''" in the String
#
# @param String:  A String to be converted
#
def ConvertToSqlString2(String):
    return String.replace("'", "''")

#
# Remove comment block
#
def RemoveBlockComment(Lines):
    IsFindBlockComment = False
    IsFindBlockCode = False
    ReservedLine = ''
    NewLines = []

    for Line in Lines:
        Line = Line.strip()
        #
        # Remove comment block
        #
        if Line.find(DataType.TAB_COMMENT_EDK_START) > -1:
            ReservedLine = GetSplitList(Line, DataType.TAB_COMMENT_EDK_START, 1)[0]
            IsFindBlockComment = True
        if Line.find(DataType.TAB_COMMENT_EDK_END) > -1:
            Line = ReservedLine + GetSplitList(Line, DataType.TAB_COMMENT_EDK_END, 1)[1]
            ReservedLine = ''
            IsFindBlockComment = False
        if IsFindBlockComment:
            NewLines.append('')
            continue

        NewLines.append(Line)
    return NewLines

#
# Get String of a List
#
def GetStringOfList(List, Split=' '):
    if type(List) != type([]):
        return List
    Str = ''
    for Item in List:
        Str = Str + Item + Split

    return Str.strip()

#
# Get HelpTextList from HelpTextClassList
#
def GetHelpTextList(HelpTextClassList):
    List = []
    if HelpTextClassList:
        for HelpText in HelpTextClassList:
            if HelpText.String.endswith('\n'):
                HelpText.String = HelpText.String[0: len(HelpText.String) - len('\n')]
                List.extend(HelpText.String.split('\n'))

    return List

def StringToArray(String):
    if isinstance(String, unicode):
        if len(unicode) == 0:
            return "{0x00, 0x00}"
        return "{%s, 0x00, 0x00}" % ", ".join(["0x%02x, 0x00" % ord(C) for C in String])
    elif String.startswith('L"'):
        if String == "L\"\"":
            return "{0x00, 0x00}"
        else:
            return "{%s, 0x00, 0x00}" % ", ".join(["0x%02x, 0x00" % ord(C) for C in String[2:-1]])
    elif String.startswith('"'):
        if String == "\"\"":
            return "{0x00}";
        else:
            return "{%s, 0x00}" % ", ".join(["0x%02x" % ord(C) for C in String[1:-1]])
    else:
        return '{%s, 0}' % ', '.join(String.split())

def StringArrayLength(String):
    if isinstance(String, unicode):
        return (len(String) + 1) * 2 + 1;
    elif String.startswith('L"'):
        return (len(String) - 3 + 1) * 2
    elif String.startswith('"'):
        return (len(String) - 2 + 1)
    else:
        return len(String.split()) + 1

def RemoveDupOption(OptionString, Which="/I", Against=None):
    OptionList = OptionString.split()
    ValueList = []
    if Against:
        ValueList += Against
    for Index in range(len(OptionList)):
        Opt = OptionList[Index]
        if not Opt.startswith(Which):
            continue
        if len(Opt) > len(Which):
            Val = Opt[len(Which):]
        else:
            Val = ""
        if Val in ValueList:
            OptionList[Index] = ""
        else:
            ValueList.append(Val)
    return " ".join(OptionList)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass

