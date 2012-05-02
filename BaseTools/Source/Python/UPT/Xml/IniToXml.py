## @file
# This file is for converting package information data file to xml file.
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
IniToXml
'''

import os.path
import re
from time import strftime
from time import localtime

import Logger.Log as Logger
from Logger.ToolError import UPT_INI_PARSE_ERROR
from Logger.ToolError import FILE_NOT_FOUND
from Library.Xml.XmlRoutines import CreateXmlElement
from Library.DataType import TAB_VALUE_SPLIT
from Library.DataType import TAB_EQUAL_SPLIT
from Library.DataType import TAB_SECTION_START
from Library.DataType import TAB_SECTION_END
from Logger import StringTable as ST
from Library.String import ConvertSpecialChar
from Library.ParserValidate import IsValidPath

## log error:
#
# @param error: error
# @param File: File
# @param Line: Line
#
def IniParseError(Error, File, Line):
    Logger.Error("UPT", UPT_INI_PARSE_ERROR, File=File,
                 Line=Line, ExtraData=Error)

## __ValidatePath
#
# @param Path: Path to be checked
#
def __ValidatePath(Path, Root):
    Path = Path.strip()
    if os.path.isabs(Path) or not IsValidPath(Path, Root):
        return False, ST.ERR_FILELIST_LOCATION % (Root, Path)
    return True, ''

## ValidateMiscFile
#
# @param Filename: File to be checked
#
def ValidateMiscFile(Filename):
    Root = ''
    if 'WORKSPACE' in os.environ:
        Root = os.environ['WORKSPACE']
    return __ValidatePath(Filename, Root)

## ValidateToolsFile
#
# @param Filename: File to be checked
#
def ValidateToolsFile(Filename):
    Valid, Cause = False, ''
    if not Valid and 'EDK_TOOLS_PATH' in os.environ:
        Valid, Cause = __ValidatePath(Filename, os.environ['EDK_TOOLS_PATH'])
    if not Valid and 'WORKSPACE' in os.environ:
        Valid, Cause = __ValidatePath(Filename, os.environ['WORKSPACE'])
    return Valid, Cause

## ParseFileList
#
# @param Line: Line
# @param Map: Map
# @param CurrentKey: CurrentKey
# @param PathFunc: Path validate function
#
def ParseFileList(Line, Map, CurrentKey, PathFunc):
    FileList = ["", {}]
    TokenList = Line.split(TAB_VALUE_SPLIT)
    if len(TokenList) > 0:
        Path = TokenList[0].strip().replace('\\', '/')
        if not Path:
            return False, ST.ERR_WRONG_FILELIST_FORMAT
        Valid, Cause = PathFunc(Path)
        if not Valid:
            return Valid, Cause
        FileList[0] = TokenList[0].strip()
        for Token in TokenList[1:]:
            Attr = Token.split(TAB_EQUAL_SPLIT)
            if len(Attr) != 2 or not Attr[0].strip() or not Attr[1].strip():
                return False, ST.ERR_WRONG_FILELIST_FORMAT
            
            Key = Attr[0].strip()
            Val = Attr[1].strip()
            if Key not in ['OS', 'Executable']:
                return False, ST.ERR_UNKNOWN_FILELIST_ATTR % Key
            
            if Key == 'OS' and Val not in ["Win32", "Win64", "Linux32", 
                                           "Linux64", "OS/X32", "OS/X64", 
                                           "GenericWin", "GenericNix"]:
                return False, ST.ERR_FILELIST_ATTR % 'OS'
            elif Key == 'Executable' and Val not in ['true', 'false']:
                return False, ST.ERR_FILELIST_ATTR % 'Executable'
            FileList[1][Key] = Val
        
        Map[CurrentKey].append(FileList)
    return True, ''

## Create header XML file
#
# @param DistMap: DistMap
# @param Root: Root
#
def CreateHeaderXml(DistMap, Root):
    Element1 = CreateXmlElement('Name', DistMap['Name'],
                                [], [['BaseName', DistMap['BaseName']]])
    Element2 = CreateXmlElement('GUID', DistMap['GUID'],
                                [], [['Version', DistMap['Version']]])
    AttributeList = [['ReadOnly', DistMap['ReadOnly']],
                     ['RePackage', DistMap['RePackage']]]
    NodeList = [Element1,
                Element2,
                ['Vendor', DistMap['Vendor']],
                ['Date', DistMap['Date']],
                ['Copyright', DistMap['Copyright']],
                ['License', DistMap['License']],
                ['Abstract', DistMap['Abstract']],
                ['Description', DistMap['Description']],
                ['Signature', DistMap['Signature']],
                ['XmlSpecification', DistMap['XmlSpecification']],
                ]
    Root.appendChild(CreateXmlElement('DistributionHeader', '',
                                      NodeList, AttributeList))

## Create tools XML file
#
# @param Map: Map
# @param Root: Root
# @param Tag: Tag 
#
def CreateToolsXml(Map, Root, Tag):
    #
    # Check if all elements in this section are empty
    #
    for Key in Map:
        if len(Map[Key]) > 0:
            break
    else:
        return

    NodeList = [['Name', Map['Name']],
                ['Copyright', Map['Copyright']],
                ['License', Map['License']],
                ['Abstract', Map['Abstract']],
                ['Description', Map['Description']],
               ]
    HeaderNode = CreateXmlElement('Header', '', NodeList, [])
    NodeList = [HeaderNode]

    for File in Map['FileList']:
        AttrList = []
        for Key in File[1]:
            AttrList.append([Key, File[1][Key]])
        NodeList.append(CreateXmlElement('Filename', File[0], [], AttrList))
    Root.appendChild(CreateXmlElement(Tag, '', NodeList, []))

## ValidateValues
#
# @param Key: Key
# @param Value: Value
# @param SectionName: SectionName
#
def ValidateValues(Key, Value, SectionName):
    if SectionName == 'DistributionHeader':
        Valid, Cause = ValidateRegValues(Key, Value)
        if not Valid:
            return Valid, Cause
        Valid = __ValidateDistHeader(Key, Value)
        if not Valid:
            return Valid, ST.ERR_VALUE_INVALID % (Key, SectionName)
    else:
        Valid = __ValidateOtherHeader(Key, Value)
        if not Valid:
            return Valid, ST.ERR_VALUE_INVALID % (Key, SectionName)
    return True, ''

## ValidateRegValues
#
# @param Key: Key
# @param Value: Value
#
def ValidateRegValues(Key, Value):
    ValidateMap = {
        'ReadOnly'  :
            ('true|false', ST.ERR_BOOLEAN_VALUE % (Key, Value)),
        'RePackage' :
            ('true|false', ST.ERR_BOOLEAN_VALUE % (Key, Value)),
        'GUID'      :
            ('[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}'
            '-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}',
            ST.ERR_GUID_VALUE % Value),
        'Version'   :   ('[0-9]+(\.[0-9]+)?', ST.ERR_VERSION_VALUE % \
                         (Key, Value)),
        'XmlSpecification' : ('1\.1', ST.ERR_VERSION_XMLSPEC % Value)
    }
    if Key not in ValidateMap:
        return True, ''
    Elem = ValidateMap[Key]
    Match = re.compile(Elem[0]).match(Value)
    if Match and Match.start() == 0 and Match.end() == len(Value):
        return True, ''
    return False, Elem[1]

## __ValidateDistHeaderName
#
# @param Name: Name
#
def __ValidateDistHeaderName(Name):
    if len(Name) < 1:
        return False
    
    for Char in Name:
        if ord(Char) < 0x20 or ord(Char) >= 0x7f:
            return False
    return True

## __ValidateDistHeaderBaseName
#
# @param BaseName: BaseName
#
def __ValidateDistHeaderBaseName(BaseName):
    if not BaseName:
        return False
#    if CheckLen and len(BaseName) < 2:
#        return False
    if not BaseName[0].isalnum() and BaseName[0] != '_':
        return False
    for Char in BaseName[1:]:
        if not Char.isalnum() and Char not in '-_':
            return False
    return True

## __ValidateDistHeaderAbstract
#
# @param Abstract: Abstract
#
def __ValidateDistHeaderAbstract(Abstract):
    return '\t' not in Abstract and len(Abstract.splitlines()) == 1

## __ValidateOtherHeaderAbstract
#
# @param Abstract: Abstract
#
def __ValidateOtherHeaderAbstract(Abstract):
    return __ValidateDistHeaderAbstract(Abstract)

## __ValidateDistHeader
#
# @param Key: Key
# @param Value: Value
#
def __ValidateDistHeader(Key, Value):
    ValidateMap = {
        'Name'      : __ValidateDistHeaderName,
        'BaseName'  : __ValidateDistHeaderBaseName,
        'Abstract'  : __ValidateDistHeaderAbstract,
        'Vendor'    : __ValidateDistHeaderAbstract
    }
    return not (Value and Key in ValidateMap and not ValidateMap[Key](Value))

## __ValidateOtherHeader
#
# @param Key: Key
# @param Value: Value
#
def __ValidateOtherHeader(Key, Value):
    ValidateMap = {
        'Name'      : __ValidateDistHeaderName,
        'Abstract'  : __ValidateOtherHeaderAbstract
    }
    return not (Value and Key in ValidateMap and not ValidateMap[Key](Value))

## Convert ini file to xml file
#
# @param IniFile
#
def IniToXml(IniFile):
    if not os.path.exists(IniFile):
        Logger.Error("UPT", FILE_NOT_FOUND, ST.ERR_TEMPLATE_NOTFOUND % IniFile)

    DistMap = {'ReadOnly' : '', 'RePackage' : '', 'Name' : '',
               'BaseName' : '', 'GUID' : '', 'Version' : '', 'Vendor' : '',
               'Date' : '', 'Copyright' : '', 'License' : '', 'Abstract' : '',
               'Description' : '', 'Signature' : '', 'XmlSpecification' : ''
                }

    ToolsMap = {'Name' : '', 'Copyright' : '', 'License' : '',
                'Abstract' : '', 'Description' : '', 'FileList' : []}
    #
    # Only FileList is a list: [['file1', {}], ['file2', {}], ...]
    #
    MiscMap = {'Name' : '', 'Copyright' : '', 'License' : '',
               'Abstract' : '', 'Description' : '', 'FileList' : []}

    SectionMap = {
                   'DistributionHeader' : DistMap,
                   'ToolsHeader' : ToolsMap,
                   'MiscellaneousFilesHeader' : MiscMap
                   }
    
    PathValidator = {
                'ToolsHeader' : ValidateToolsFile,
                'MiscellaneousFilesHeader' : ValidateMiscFile
                }
    
    ParsedSection = []

    SectionName = ''
    CurrentKey = ''
    PreMap = None
    Map = None
    FileContent = ConvertSpecialChar(open(IniFile, 'rb').readlines())
    LastIndex = 0
    for Index in range(0, len(FileContent)):
        LastIndex = Index
        Line = FileContent[Index].strip()
        if Line == '':
            continue
        if Line[0] == TAB_SECTION_START and Line[-1] == TAB_SECTION_END:
            CurrentKey = ''
            SectionName = Line[1:-1].strip()
            if SectionName not in SectionMap:
                IniParseError(ST.ERR_SECTION_NAME_INVALID % SectionName,
                      IniFile, Index+1)
            
            if SectionName in ParsedSection:
                IniParseError(ST.ERR_SECTION_REDEFINE % SectionName,
                      IniFile, Index+1)
            else:
                ParsedSection.append(SectionName)
            
            Map = SectionMap[SectionName]
            continue
        if not Map:
            IniParseError(ST.ERR_SECTION_NAME_NONE, IniFile, Index+1)
        TokenList = Line.split(TAB_EQUAL_SPLIT, 1)
        TempKey = TokenList[0].strip()
        #
        # Value spanned multiple or same keyword appears more than one time
        #
        if len(TokenList) < 2 or TempKey not in Map:
            if CurrentKey == '':
                IniParseError(ST.ERR_KEYWORD_INVALID % TempKey,
                              IniFile, Index+1)
            elif CurrentKey == 'FileList':
                #
                # Special for FileList
                #
                Valid, Cause = ParseFileList(Line, Map, CurrentKey, 
                                             PathValidator[SectionName])
                if not Valid:
                    IniParseError(Cause, IniFile, Index+1)

            else:
                #
                # Multiple lines for one key such as license
                # Or if string on the left side of '=' is not a keyword
                #
                Map[CurrentKey] = ''.join([Map[CurrentKey], '\n', Line])
                Valid, Cause = ValidateValues(CurrentKey, 
                                              Map[CurrentKey], SectionName)
                if not Valid:
                    IniParseError(Cause, IniFile, Index+1)
            continue

        if (TokenList[1].strip() == ''):
            IniParseError(ST.ERR_EMPTY_VALUE, IniFile, Index+1)

        #
        # A keyword found
        #
        CurrentKey = TempKey
        if Map[CurrentKey]:
            IniParseError(ST.ERR_KEYWORD_REDEFINE % CurrentKey,
                          IniFile, Index+1)
        
        if id(Map) != id(PreMap) and Map['Copyright']:
            PreMap = Map
            Copyright = Map['Copyright'].lower()
            Pos = Copyright.find('copyright')
            if Pos == -1:
                IniParseError(ST.ERR_COPYRIGHT_CONTENT, IniFile, Index)
            if not Copyright[Pos + len('copyright'):].lstrip(' ').startswith('('):
                IniParseError(ST.ERR_COPYRIGHT_CONTENT, IniFile, Index)
        
        if CurrentKey == 'FileList':
            Valid, Cause = ParseFileList(TokenList[1], Map, CurrentKey, 
                                         PathValidator[SectionName])
            if not Valid:
                IniParseError(Cause, IniFile, Index+1)
        else:
            Map[CurrentKey] = TokenList[1].strip()
            Valid, Cause = ValidateValues(CurrentKey,
                                          Map[CurrentKey], SectionName)
            if not Valid:
                IniParseError(Cause, IniFile, Index+1)
    
    if id(Map) != id(PreMap) and Map['Copyright'] and 'copyright' not in Map['Copyright'].lower():
        IniParseError(ST.ERR_COPYRIGHT_CONTENT, IniFile, LastIndex)

    #
    # Check mandatory keys
    #    
    CheckMdtKeys(DistMap, IniFile, LastIndex, 
                 (('ToolsHeader', ToolsMap), ('MiscellaneousFilesHeader', MiscMap))
                 )
    
    return CreateXml(DistMap, ToolsMap, MiscMap, IniFile)


## CheckMdtKeys
#
# @param MdtDistKeys: All mandatory keys
# @param DistMap: Dist content
# @param IniFile: Ini file
# @param LastIndex: Last index of Ini file
# @param Maps: Tools and Misc section name and map. (('section_name', map),*)
#
def CheckMdtKeys(DistMap, IniFile, LastIndex, Maps):    
    MdtDistKeys = ['Name', 'GUID', 'Version', 'Vendor', 'Copyright', 'License', 'Abstract', 'XmlSpecification']
    for Key in MdtDistKeys:
        if Key not in DistMap or DistMap[Key] == '':
            IniParseError(ST.ERR_KEYWORD_MANDATORY % Key, IniFile, LastIndex+1)
    
    if '.' not in DistMap['Version']:
        DistMap['Version'] = DistMap['Version'] + '.0'
    
    DistMap['Date'] = str(strftime("%Y-%m-%dT%H:%M:%S", localtime()))

    #
    # Check Tools Surface Area according to UPT Spec
    # <Tools> {0,}
    #     <Header> ... </Header> {0,1}
    #     <Filename> ... </Filename> {1,}
    # </Tools>
    # <Header>
    #    <Name> xs:normalizedString </Name> {1}
    #    <Copyright> xs:string </Copyright> {0,1}
    #    <License> xs:string </License> {0,1}
    #    <Abstract> xs:normalizedString </Abstract> {0,1}
    #    <Description> xs:string </Description> {0,1}
    # </Header>
    #
    for Item in Maps:
        Map = Item[1]
        NonEmptyKey = 0
        for Key in Map:
            if Map[Key]:
                NonEmptyKey += 1
        
        if NonEmptyKey > 0 and not Map['FileList']:
            IniParseError(ST.ERR_KEYWORD_MANDATORY % (Item[0] + '.FileList'), IniFile, LastIndex+1)
        
        if NonEmptyKey > 0 and not Map['Name']:
            IniParseError(ST.ERR_KEYWORD_MANDATORY % (Item[0] + '.Name'), IniFile, LastIndex+1)

## CreateXml
#
# @param DistMap:  Dist Content
# @param ToolsMap: Tools Content
# @param MiscMap:  Misc Content
# @param IniFile:  Ini File
#
def CreateXml(DistMap, ToolsMap, MiscMap, IniFile):    
    Attrs = [['xmlns', 'http://www.uefi.org/2011/1.1'],
             ['xmlns:xsi', 'http:/www.w3.org/2001/XMLSchema-instance'],
            ]
    Root = CreateXmlElement('DistributionPackage', '', [], Attrs)
    CreateHeaderXml(DistMap, Root)
    CreateToolsXml(ToolsMap, Root, 'Tools')
    CreateToolsXml(MiscMap, Root, 'MiscellaneousFiles')

    FileAndExt = IniFile.rsplit('.', 1)
    if len(FileAndExt) > 1:
        FileName = FileAndExt[0] + '.xml'
    else:
        FileName = IniFile + '.xml'
    File = open(FileName, 'w')
    
    try:
        File.write(Root.toprettyxml(indent = '  '))
    finally:
        File.close()
    return FileName

