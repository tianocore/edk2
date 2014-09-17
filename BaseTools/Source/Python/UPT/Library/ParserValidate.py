## @file ParserValidate.py
# Functions for parser validation
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
PaserValidate
'''

import os.path
import re
import platform

from Library.DataType import MODULE_LIST
from Library.DataType import COMPONENT_TYPE_LIST
from Library.DataType import PCD_USAGE_TYPE_LIST_OF_MODULE
from Library.DataType import TAB_SPACE_SPLIT
from Library.String import GetSplitValueList
from Library.ExpressionValidate import IsValidBareCString
from Library.ExpressionValidate import IsValidFeatureFlagExp

## __HexDigit() method
#
# Whether char input is a Hex data bit
#
# @param  TempChar:    The char to test
#
def __HexDigit(TempChar):
    if (TempChar >= 'a' and TempChar <= 'f') or \
    (TempChar >= 'A' and TempChar <= 'F') \
            or (TempChar >= '0' and TempChar <= '9'):
        return True
    else:
        return False
 
## IsValidHex() method
#
# Whether char input is a Hex data.
#
# @param  TempChar:    The char to test
#
def IsValidHex(HexStr):
    if not HexStr.upper().startswith("0X"):
        return False
    CharList = [c for c in HexStr[2:] if not __HexDigit(c)]
    if len(CharList) == 0:
        return True
    else:
        return False

## Judge the input string is valid bool type or not.
# 
# <TRUE>                  ::=  {"TRUE"} {"true"} {"True"} {"0x1"} {"0x01"}
# <FALSE>                 ::=  {"FALSE"} {"false"} {"False"} {"0x0"} {"0x00"}
# <BoolType>              ::=  {<TRUE>} {<FALSE>}
#
# @param    BoolString:    A string contained the value need to be judged.
#
def IsValidBoolType(BoolString):
    #
    # Valid Ture
    #
    if BoolString == 'TRUE' or \
       BoolString == 'True' or \
       BoolString == 'true' or \
       BoolString == '0x1' or \
       BoolString == '0x01':
        return True
    #
    # Valid False
    #
    elif BoolString == 'FALSE' or \
         BoolString == 'False' or \
         BoolString == 'false' or \
         BoolString == '0x0' or \
         BoolString == '0x00':
        return True
    #
    # Invalid bool type
    #
    else:
        return False
    
## Is Valid Module Type List or not 
#    
# @param      ModuleTypeList:  A list contain ModuleType strings need to be 
# judged.
#
def IsValidInfMoudleTypeList(ModuleTypeList):
    for ModuleType in ModuleTypeList:
        return IsValidInfMoudleType(ModuleType)

## Is Valid Module Type or not 
#    
# @param      ModuleType:  A string contain ModuleType need to be judged.
#
def IsValidInfMoudleType(ModuleType):
    if ModuleType in MODULE_LIST:
        return True
    else:
        return False

## Is Valid Component Type or not 
#    
# @param      ComponentType:  A string contain ComponentType need to be judged.
#
def IsValidInfComponentType(ComponentType):
    if ComponentType.upper() in COMPONENT_TYPE_LIST:
        return True
    else:
        return False


## Is valid Tool Family or not
#
# @param   ToolFamily:   A string contain Tool Family need to be judged.
# Famlily := [A-Z]([a-zA-Z0-9])* 
#
def IsValidToolFamily(ToolFamily):
    ReIsValieFamily = re.compile(r"^[A-Z]+[A-Za-z0-9]{0,}$", re.DOTALL)
    if ReIsValieFamily.match(ToolFamily) == None:
        return False
    return True

## Is valid Tool TagName or not
#
# The TagName sample is MYTOOLS and VS2005.
#
# @param   TagName:   A string contain Tool TagName need to be judged.
#
def IsValidToolTagName(TagName):
    if TagName.strip() == '':
        return True
    if TagName.strip() == '*':
        return True
    if not IsValidWord(TagName):
        return False
    return True

## Is valid arch or not
# 
# @param Arch   The arch string need to be validated
# <OA>                  ::=  (a-zA-Z)(A-Za-z0-9){0,}
# <arch>                 ::=  {"IA32"} {"X64"} {"IPF"} {"EBC"} {<OA>}
#                            {"common"}
# @param   Arch:   Input arch
# 
def IsValidArch(Arch):
    if Arch == 'common':
        return True
    ReIsValieArch = re.compile(r"^[a-zA-Z]+[a-zA-Z0-9]{0,}$", re.DOTALL)
    if ReIsValieArch.match(Arch) == None:
        return False
    return True

## Is valid family or not
# 
# <Family>        ::=  {"MSFT"} {"GCC"} {"INTEL"} {<Usr>} {"*"}
# <Usr>           ::=  [A-Z][A-Za-z0-9]{0,}
#
# @param family:   The family string need to be validated
# 
def IsValidFamily(Family):
    Family = Family.strip()
    if Family == '*':
        return True
    
    if Family == '':
        return True
       
    ReIsValidFamily = re.compile(r"^[A-Z]+[A-Za-z0-9]{0,}$", re.DOTALL)
    if ReIsValidFamily.match(Family) == None:
        return False
    return True

## Is valid build option name or not
# 
# @param BuildOptionName:   The BuildOptionName string need to be validated
#
def IsValidBuildOptionName(BuildOptionName):
    if not BuildOptionName:
        return False
    
    ToolOptionList = GetSplitValueList(BuildOptionName, '_', 4)
    
    if len(ToolOptionList) != 5:
        return False
    
    ReIsValidBuildOption1 = re.compile(r"^\s*(\*)|([A-Z][a-zA-Z0-9]*)$")
    ReIsValidBuildOption2 = re.compile(r"^\s*(\*)|([a-zA-Z][a-zA-Z0-9]*)$")
    
    if ReIsValidBuildOption1.match(ToolOptionList[0]) == None:
        return False
    
    if ReIsValidBuildOption1.match(ToolOptionList[1]) == None:
        return False
    
    if ReIsValidBuildOption2.match(ToolOptionList[2]) == None:
        return False
    
    if ToolOptionList[3] == "*" and ToolOptionList[4] not in ['FAMILY', 'DLL', 'DPATH']:
        return False
           
    return True
    
## IsValidToken
#
# Check if pattern string matches total token
#
# @param ReString:     regular string
# @param Token:        Token to be matched
#
def IsValidToken(ReString, Token):
    Match = re.compile(ReString).match(Token)
    return Match and Match.start() == 0 and Match.end() == len(Token)

## IsValidPath
#
# Check if path exist
#
# @param Path: Absolute path or relative path to be checked
# @param Root: Root path
#
def IsValidPath(Path, Root):
    Path = Path.strip()
    OrigPath = Path.replace('\\', '/')
    
    Path = os.path.normpath(Path).replace('\\', '/')
    Root = os.path.normpath(Root).replace('\\', '/')
    FullPath = os.path.normpath(os.path.join(Root, Path)).replace('\\', '/')
    
    if not os.path.exists(FullPath):
        return False
    
    #
    # If Path is absolute path.
    # It should be in Root.
    #
    if os.path.isabs(Path):
        if not Path.startswith(Root):
            return False
        return True

    #
    # Check illegal character
    #
    for Rel in ['/', './', '../']:
        if OrigPath.startswith(Rel):
            return False
    for Rel in ['//', '/./', '/../']:
        if Rel in OrigPath:
            return False
    for Rel in ['/.', '/..', '/']:
        if OrigPath.endswith(Rel):
            return False
    
    Path = Path.rstrip('/')
    
    #
    # Check relative path
    #
    for Word in Path.split('/'):
        if not IsValidWord(Word):
            return False
    
    return True

## IsValidInstallPath
#
# Check if an install path valid or not.
#
# Absolute path or path starts with '.' or path contains '..' are invalid.
#
# @param Path: path to be checked
#
def IsValidInstallPath(Path):
    if platform.platform().find("Windows") >= 0:
        if os.path.isabs(Path):
            return False
    else:
        if Path[1:2] == ':':
            return False
        if os.path.isabs(Path):
            return False
    if Path.startswith('.'):
        return False
    
    if Path.find('..') != -1:
        return False
    
    return True
    

## IsValidCFormatGuid
#
# Check if GUID format has the from of {8,4,4,{2,2,2,2,2,2,2,2}}
#
# @param Guid: Guid to be checked
#
def IsValidCFormatGuid(Guid):
    #
    # Valid: { 0xf0b11735, 0x87a0, 0x4193, {0xb2, 0x66, 0x53, 0x8c, 0x38, 
    #        0xaf, 0x48, 0xce }}
    # Invalid: { 0xf0b11735, 0x87a0, 0x4193, {0xb2, 0x66, 0x53, 0x8c, 0x38, 
    #          0xaf, 0x48, 0xce }} 0x123
    # Invalid: { 0xf0b1 1735, 0x87a0, 0x4193, {0xb2, 0x66, 0x53, 0x8c, 0x38, 
    #          0xaf, 0x48, 0xce }}
    #
    List = ['{', 10, ',', 6, ',', 6, ',{', 4, ',', 4, ',', 4, 
            ',', 4, ',', 4, ',', 4, ',', 4, ',', 4, '}}']
    Index = 0
    Value = ''
    SepValue = ''
    for Char in Guid:
        if Char not in '{},\t ':
            Value += Char
            continue
        if Value:
            try:
                #
                # Index may out of bound
                #
                if not SepValue or SepValue != List[Index]:
                    return False
                Index += 1
                SepValue = ''

                if not Value.startswith('0x') and not Value.startswith('0X'):
                    return False
                
                #
                # Index may out of bound
                #
                if type(List[Index]) != type(1) or \
                   len(Value) > List[Index] or len(Value) < 3:
                    return False
                
                #
                # Check if string can be converted to integer
                # Throw exception if not
                #
                int(Value, 16)
            except BaseException:
                #
                # Exception caught means invalid format
                #
                return False
            Value = ''
            Index += 1
        if Char in '{},':
            SepValue += Char

    return SepValue == '}}' and Value == ''

## IsValidPcdType
#
# Check whether the PCD type is valid
#
# @param PcdTypeString: The PcdType string need to be checked.
#    
def IsValidPcdType(PcdTypeString):
    if PcdTypeString.upper() in PCD_USAGE_TYPE_LIST_OF_MODULE:
        return True
    else:
        return False
    
## IsValidWord
#
# Check whether the word is valid.
# <Word>   ::=  (a-zA-Z0-9_)(a-zA-Z0-9_-){0,} Alphanumeric characters with 
#               optional 
#               dash "-" and/or underscore "_" characters. No whitespace 
#               characters are permitted.
#              
# @param Word:  The word string need to be checked.
#    
def IsValidWord(Word):
    if not Word:
        return False
    #
    # The first char should be alpha, _ or Digit.
    #
    if not Word[0].isalnum() and \
       not Word[0] == '_' and \
       not Word[0].isdigit():
        return False
    
    LastChar = ''
    for Char in Word[1:]:      
        if (not Char.isalpha()) and \
           (not Char.isdigit()) and \
           Char != '-' and \
           Char != '_' and \
           Char != '.':
            return False
        if Char == '.' and LastChar == '.':
            return False
        LastChar = Char
    
    return True


## IsValidSimpleWord
#
# Check whether the SimpleWord is valid.
# <SimpleWord>          ::=  (a-zA-Z0-9)(a-zA-Z0-9_-){0,} 
#                       A word that cannot contain a period character.
#              
# @param Word:  The word string need to be checked.
#    
def IsValidSimpleWord(Word):
    ReIsValidSimpleWord = \
        re.compile(r"^[0-9A-Za-z][0-9A-Za-z\-_]*$", re.DOTALL)
    Word = Word.strip()
    if not Word:
        return False
    
    if not ReIsValidSimpleWord.match(Word):
        return False
      
    return True

## IsValidDecVersion
#
# Check whether the decimal version is valid.
# <DecVersion>          ::=  (0-9){1,} ["." (0-9){1,}]
#              
# @param Word:  The word string need to be checked.
#  
def IsValidDecVersion(Word):
    if Word.find('.') > -1:
        ReIsValidDecVersion = re.compile(r"[0-9]+\.?[0-9]+$")
    else:
        ReIsValidDecVersion = re.compile(r"[0-9]+$")
    if ReIsValidDecVersion.match(Word) == None:
        return False 
    return True
   
## IsValidHexVersion
#
# Check whether the hex version is valid.
# <HexVersion>          ::=  "0x" <Major> <Minor>
# <Major>               ::=  <HexDigit>{4}
# <Minor>               ::=  <HexDigit>{4}
#              
# @param Word:  The word string need to be checked.
#  
def IsValidHexVersion(Word):
    ReIsValidHexVersion = re.compile(r"[0][xX][0-9A-Fa-f]{8}$", re.DOTALL)
    if ReIsValidHexVersion.match(Word) == None:
        return False
    
    return True

## IsValidBuildNumber
#
# Check whether the BUILD_NUMBER is valid.
# ["BUILD_NUMBER" "=" <Integer>{1,4} <EOL>]
#              
# @param Word:  The BUILD_NUMBER string need to be checked.
#  
def IsValidBuildNumber(Word):
    ReIsValieBuildNumber = re.compile(r"[0-9]{1,4}$", re.DOTALL)
    if ReIsValieBuildNumber.match(Word) == None:
        return False
    
    return True

## IsValidDepex
#
# Check whether the Depex is valid.
#              
# @param Word:  The Depex string need to be checked.
# 
def IsValidDepex(Word):
    Index = Word.upper().find("PUSH")
    if Index > -1:
        return IsValidCFormatGuid(Word[Index+4:].strip())

    ReIsValidCName = re.compile(r"^[A-Za-z_][0-9A-Za-z_\s\.]*$", re.DOTALL)
    if ReIsValidCName.match(Word) == None:
        return False
    
    return True

## IsValidNormalizedString
#
# Check 
# <NormalizedString>    ::=  <DblQuote> [{<Word>} {<Space>}]{1,} <DblQuote>
# <Space>               ::=  0x20
#
# @param String: string to be checked
#
def IsValidNormalizedString(String):
    if String == '':
        return True
    
    for Char in String:
        if Char == '\t':
            return False
    
    StringList = GetSplitValueList(String, TAB_SPACE_SPLIT)
    
    for Item in StringList:
        if not Item:
            continue
        if not IsValidWord(Item):
            return False
    
    return True

## IsValidIdString
#
# Check whether the IdString is valid.
#              
# @param IdString:  The IdString need to be checked.
#     
def IsValidIdString(String):
    if IsValidSimpleWord(String.strip()):
        return True
    
    if String.strip().startswith('"') and \
       String.strip().endswith('"'):
        String = String[1:-1]
        if String.strip() == "":
            return True
        if IsValidNormalizedString(String):
            return True
    
    return False

## IsValidVersionString
#
# Check whether the VersionString is valid.
# <AsciiString>           ::=  [ [<WhiteSpace>]{0,} [<AsciiChars>]{0,} ] {0,}
# <WhiteSpace>            ::=  {<Tab>} {<Space>}
# <Tab>                   ::=  0x09
# <Space>                 ::=  0x20
# <AsciiChars>            ::=  (0x21 - 0x7E) 
#   
# @param VersionString:  The VersionString need to be checked.
#     
def IsValidVersionString(VersionString):
    VersionString = VersionString.strip()
    for Char in VersionString:
        if not (Char >= 0x21 and Char <= 0x7E):
            return False
    
    return True

## IsValidPcdValue
#
# Check whether the PcdValue is valid.
#   
# @param VersionString:  The PcdValue need to be checked.
#     
def IsValidPcdValue(PcdValue):
    for Char in PcdValue:
        if Char == '\n' or Char == '\t' or Char == '\f':
            return False
    
    #
    # <Boolean>
    #
    if IsValidFeatureFlagExp(PcdValue, True)[0]:
        return True
    
    #
    # <Number>                ::=  {<Integer>} {<HexNumber>}
    # <Integer>               ::=  {(0-9)} {(1-9)(0-9){1,}}
    # <HexNumber>             ::=  "0x" <HexDigit>{1,}
    # <HexDigit>              ::=  (a-fA-F0-9)
    #          
    if IsValidHex(PcdValue):
        return True
    
    ReIsValidIntegerSingle = re.compile(r"^\s*[0-9]\s*$", re.DOTALL)
    if ReIsValidIntegerSingle.match(PcdValue) != None:
        return True
    
    ReIsValidIntegerMulti = re.compile(r"^\s*[1-9][0-9]+\s*$", re.DOTALL)   
    if ReIsValidIntegerMulti.match(PcdValue) != None:
        return True
    
    #
    # <StringVal>              ::=  {<StringType>} {<Array>} {"$(" <MACRO> ")"}
    # <StringType>             ::=  {<UnicodeString>} {<CString>}
    #
    ReIsValidStringType = re.compile(r"^\s*[\"L].*[\"]\s*$")
    if ReIsValidStringType.match(PcdValue):
        IsTrue = False
        if PcdValue.strip().startswith('L\"'):
            StringValue = PcdValue.strip().lstrip('L\"').rstrip('\"')
            if IsValidBareCString(StringValue):
                IsTrue = True
        elif PcdValue.strip().startswith('\"'):
            StringValue = PcdValue.strip().lstrip('\"').rstrip('\"')
            if IsValidBareCString(StringValue):
                IsTrue = True
        if IsTrue:
            return IsTrue
    
    #
    # <Array>                 ::=   {<CArray>} {<NList>} {<CFormatGUID>}
    # <CArray>                ::=   "{" [<NList>] <CArray>{0,} "}"
    # <NList>                 ::=   <HexByte> ["," <HexByte>]{0,}
    # <HexDigit>              ::=  (a-fA-F0-9)
    # <HexByte>               ::=  "0x" <HexDigit>{1,2}
    #
    if IsValidCFormatGuid(PcdValue):
        return True
    
    ReIsValidByteHex = re.compile(r"^\s*0x[0-9a-fA-F]{1,2}\s*$", re.DOTALL)
    if PcdValue.strip().startswith('{') and PcdValue.strip().endswith('}') :
        StringValue = PcdValue.strip().lstrip('{').rstrip('}')
        ValueList = StringValue.split(',')
        AllValidFlag = True
        for ValueItem in ValueList:         
            if not ReIsValidByteHex.match(ValueItem.strip()):
                AllValidFlag = False
        
        if AllValidFlag:
            return True
    
    #    
    # NList
    #
    AllValidFlag = True
    ValueList = PcdValue.split(',')
    for ValueItem in ValueList:         
        if not ReIsValidByteHex.match(ValueItem.strip()):
            AllValidFlag = False
    
    if AllValidFlag:
        return True
    
    return False

## IsValidCVariableName
#
# Check whether the PcdValue is valid.
#   
# @param VersionString:  The PcdValue need to be checked.
#     
def IsValidCVariableName(CName):
    ReIsValidCName = re.compile(r"^[A-Za-z_][0-9A-Za-z_]*$", re.DOTALL)
    if ReIsValidCName.match(CName) == None:
        return False
    
    return True

## IsValidIdentifier
#
# <Identifier> ::= <NonDigit> <Chars>{0,}
# <Chars> ::= (a-zA-Z0-9_)
# <NonDigit> ::= (a-zA-Z_)
#
# @param Ident: identifier to be checked
#
def IsValidIdentifier(Ident):
    ReIdent = re.compile(r"^[A-Za-z_][0-9A-Za-z_]*$", re.DOTALL)
    if ReIdent.match(Ident) == None:
        return False
    
    return True

## IsValidDecVersionVal
#
# {(0-9){1,} "." (0-99)}
#
# @param Ver: version to be checked
#
def IsValidDecVersionVal(Ver):
    ReVersion = re.compile(r"[0-9]+(\.[0-9]{1,2})$")
    
    if ReVersion.match(Ver) == None:
        return False
      
    return True


## IsValidLibName
#
# (A-Z)(a-zA-Z0-9){0,} and could not be "NULL"
#
def IsValidLibName(LibName):
    if LibName == 'NULL':
        return False
    ReLibName = re.compile("^[A-Z]+[a-zA-Z0-9]*$")
    if not ReLibName.match(LibName):
        return False
    
    return True

# IsValidUserId
#
# <UserId> ::= (a-zA-Z)(a-zA-Z0-9_.){0,}
# Words that contain period "." must be encapsulated in double quotation marks.
#
def IsValidUserId(UserId):
    UserId = UserId.strip()
    Quoted = False
    if UserId.startswith('"') and UserId.endswith('"'):
        Quoted = True
        UserId = UserId[1:-1]
    if not UserId or not UserId[0].isalpha():
        return False
    for Char in UserId[1:]:
        if not Char.isalnum() and not Char in '_.':
            return False
        if Char == '.' and not Quoted:
            return False
    return True

