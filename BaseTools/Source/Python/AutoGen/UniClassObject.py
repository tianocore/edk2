# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

#
#This file is used to collect all defined strings in multiple uni files
#

##
# Import Modules
#
import os, codecs, re
import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
from Common.String import GetLineNo
from Common.Misc import PathClass

##
# Static definitions
#
UNICODE_WIDE_CHAR = u'\\wide'
UNICODE_NARROW_CHAR = u'\\narrow'
UNICODE_NON_BREAKING_CHAR = u'\\nbr'
UNICODE_UNICODE_CR = '\r'
UNICODE_UNICODE_LF = '\n'

NARROW_CHAR = u'\uFFF0'
WIDE_CHAR = u'\uFFF1'
NON_BREAKING_CHAR = u'\uFFF2'
CR = u'\u000D'
LF = u'\u000A'
NULL = u'\u0000'
TAB = u'\t'
BACK_SPLASH = u'\\'

gIncludePattern = re.compile("^#include +[\"<]+([^\"< >]+)[>\"]+$", re.MULTILINE | re.UNICODE)

## Convert a python unicode string to a normal string
#
# Convert a python unicode string to a normal string
# UniToStr(u'I am a string') is 'I am a string'
#
# @param Uni:  The python unicode string
#
# @retval:     The formatted normal string
#
def UniToStr(Uni):
    return repr(Uni)[2:-1]

## Convert a unicode string to a Hex list
#
# Convert a unicode string to a Hex list
# UniToHexList('ABC') is ['0x41', '0x00', '0x42', '0x00', '0x43', '0x00']
#
# @param Uni:    The python unicode string
#
# @retval List:  The formatted hex list
#
def UniToHexList(Uni):
    List = []
    for Item in Uni:
        Temp = '%04X' % ord(Item)
        List.append('0x' + Temp[2:4])
        List.append('0x' + Temp[0:2])
    return List

LangConvTable = {'eng':'en', 'fra':'fr', \
                 'aar':'aa', 'abk':'ab', 'ave':'ae', 'afr':'af', 'aka':'ak', 'amh':'am', \
                 'arg':'an', 'ara':'ar', 'asm':'as', 'ava':'av', 'aym':'ay', 'aze':'az', \
                 'bak':'ba', 'bel':'be', 'bul':'bg', 'bih':'bh', 'bis':'bi', 'bam':'bm', \
                 'ben':'bn', 'bod':'bo', 'bre':'br', 'bos':'bs', 'cat':'ca', 'che':'ce', \
                 'cha':'ch', 'cos':'co', 'cre':'cr', 'ces':'cs', 'chu':'cu', 'chv':'cv', \
                 'cym':'cy', 'dan':'da', 'deu':'de', 'div':'dv', 'dzo':'dz', 'ewe':'ee', \
                 'ell':'el', 'epo':'eo', 'spa':'es', 'est':'et', 'eus':'eu', 'fas':'fa', \
                 'ful':'ff', 'fin':'fi', 'fij':'fj', 'fao':'fo', 'fry':'fy', 'gle':'ga', \
                 'gla':'gd', 'glg':'gl', 'grn':'gn', 'guj':'gu', 'glv':'gv', 'hau':'ha', \
                 'heb':'he', 'hin':'hi', 'hmo':'ho', 'hrv':'hr', 'hat':'ht', 'hun':'hu', \
                 'hye':'hy', 'her':'hz', 'ina':'ia', 'ind':'id', 'ile':'ie', 'ibo':'ig', \
                 'iii':'ii', 'ipk':'ik', 'ido':'io', 'isl':'is', 'ita':'it', 'iku':'iu', \
                 'jpn':'ja', 'jav':'jv', 'kat':'ka', 'kon':'kg', 'kik':'ki', 'kua':'kj', \
                 'kaz':'kk', 'kal':'kl', 'khm':'km', 'kan':'kn', 'kor':'ko', 'kau':'kr', \
                 'kas':'ks', 'kur':'ku', 'kom':'kv', 'cor':'kw', 'kir':'ky', 'lat':'la', \
                 'ltz':'lb', 'lug':'lg', 'lim':'li', 'lin':'ln', 'lao':'lo', 'lit':'lt', \
                 'lub':'lu', 'lav':'lv', 'mlg':'mg', 'mah':'mh', 'mri':'mi', 'mkd':'mk', \
                 'mal':'ml', 'mon':'mn', 'mar':'mr', 'msa':'ms', 'mlt':'mt', 'mya':'my', \
                 'nau':'na', 'nob':'nb', 'nde':'nd', 'nep':'ne', 'ndo':'ng', 'nld':'nl', \
                 'nno':'nn', 'nor':'no', 'nbl':'nr', 'nav':'nv', 'nya':'ny', 'oci':'oc', \
                 'oji':'oj', 'orm':'om', 'ori':'or', 'oss':'os', 'pan':'pa', 'pli':'pi', \
                 'pol':'pl', 'pus':'ps', 'por':'pt', 'que':'qu', 'roh':'rm', 'run':'rn', \
                 'ron':'ro', 'rus':'ru', 'kin':'rw', 'san':'sa', 'srd':'sc', 'snd':'sd', \
                 'sme':'se', 'sag':'sg', 'sin':'si', 'slk':'sk', 'slv':'sl', 'smo':'sm', \
                 'sna':'sn', 'som':'so', 'sqi':'sq', 'srp':'sr', 'ssw':'ss', 'sot':'st', \
                 'sun':'su', 'swe':'sv', 'swa':'sw', 'tam':'ta', 'tel':'te', 'tgk':'tg', \
                 'tha':'th', 'tir':'ti', 'tuk':'tk', 'tgl':'tl', 'tsn':'tn', 'ton':'to', \
                 'tur':'tr', 'tso':'ts', 'tat':'tt', 'twi':'tw', 'tah':'ty', 'uig':'ug', \
                 'ukr':'uk', 'urd':'ur', 'uzb':'uz', 'ven':'ve', 'vie':'vi', 'vol':'vo', \
                 'wln':'wa', 'wol':'wo', 'xho':'xh', 'yid':'yi', 'yor':'yo', 'zha':'za', \
                 'zho':'zh', 'zul':'zu'}

## GetLanguageCode
#
# Check the language code read from .UNI file and convert ISO 639-2 codes to RFC 4646 codes if appropriate
# ISO 639-2 language codes supported in compatiblity mode
# RFC 4646 language codes supported in native mode
#
# @param LangName:   Language codes read from .UNI file
#
# @retval LangName:  Valid lanugage code in RFC 4646 format or None
#
def GetLanguageCode(LangName, IsCompatibleMode, File):
    global LangConvTable

    length = len(LangName)
    if IsCompatibleMode:
        if length == 3 and LangName.isalpha():
            TempLangName = LangConvTable.get(LangName.lower())
            if TempLangName != None:
               return TempLangName
            return LangName
        else:
            EdkLogger.error("Unicode File Parser", FORMAT_INVALID, "Invalid ISO 639-2 language code : %s" % LangName, File)

    if length == 2:
        if LangName.isalpha():
            return LangName
    elif length == 3:
        if LangName.isalpha() and LangConvTable.get(LangName.lower()) == None:
            return LangName
    elif length == 5:
        if LangName[0:2].isalpha() and LangName[2] == '-':
            return LangName
    elif length >= 6:
        if LangName[0:2].isalpha() and LangName[2] == '-':
            return LangName
        if LangName[0:3].isalpha() and LangConvTable.get(LangName.lower()) == None and LangName[3] == '-':
            return LangName

    EdkLogger.error("Unicode File Parser", FORMAT_INVALID, "Invalid RFC 4646 language code : %s" % LangName, File)

## StringDefClassObject
#
# A structure for language definition
#
class StringDefClassObject(object):
    def __init__(self, Name = None, Value = None, Referenced = False, Token = None, UseOtherLangDef = ''):
        self.StringName = ''
        self.StringNameByteList = []
        self.StringValue = ''
        self.StringValueByteList = ''
        self.Token = 0
        self.Referenced = Referenced
        self.UseOtherLangDef = UseOtherLangDef
        self.Length = 0

        if Name != None:
            self.StringName = Name
            self.StringNameByteList = UniToHexList(Name)
        if Value != None:
            self.StringValue = Value + u'\x00'        # Add a NULL at string tail
            self.StringValueByteList = UniToHexList(self.StringValue)
            self.Length = len(self.StringValueByteList)
        if Token != None:
            self.Token = Token

    def __str__(self):
        return repr(self.StringName) + ' ' + \
               repr(self.Token) + ' ' + \
               repr(self.Referenced) + ' ' + \
               repr(self.StringValue) + ' ' + \
               repr(self.UseOtherLangDef)

## UniFileClassObject
#
# A structure for .uni file definition
#
class UniFileClassObject(object):
    def __init__(self, FileList = [], IsCompatibleMode = False):
        self.FileList = FileList
        self.Token = 2
        self.LanguageDef = []                   #[ [u'LanguageIdentifier', u'PrintableName'], ... ]
        self.OrderedStringList = {}             #{ u'LanguageIdentifier' : [StringDefClassObject]  }
        self.IsCompatibleMode = IsCompatibleMode

        if len(self.FileList) > 0:
            self.LoadUniFiles(FileList)

    #
    # Get Language definition
    #
    def GetLangDef(self, File, Line):
        Lang = Line.split()
        if len(Lang) != 3:
            try:
                FileIn = codecs.open(File, mode='rb', encoding='utf-16').read()
            except UnicodeError, X:
                EdkLogger.error("build", FILE_READ_FAILURE, "File read failure: %s" % str(X), ExtraData=File);
            except:
                EdkLogger.error("build", FILE_OPEN_FAILURE, ExtraData=File);
            LineNo = GetLineNo(FileIn, Line, False)
            EdkLogger.error("Unicode File Parser", PARSER_ERROR, "Wrong language definition",
                            ExtraData="""%s\n\t*Correct format is like '#langdef eng "English"'""" % Line, File = File, Line = LineNo)
        else:
            LangName = GetLanguageCode(Lang[1], self.IsCompatibleMode, self.File)
            LangPrintName = Lang[2][1:-1]

        IsLangInDef = False
        for Item in self.LanguageDef:
            if Item[0] == LangName:
                IsLangInDef = True
                break;

        if not IsLangInDef:
            self.LanguageDef.append([LangName, LangPrintName])

        #
        # Add language string
        #
        self.AddStringToList(u'$LANGUAGE_NAME', LangName, LangName, 0, True, Index=0)
        self.AddStringToList(u'$PRINTABLE_LANGUAGE_NAME', LangName, LangPrintName, 1, True, Index=1)

        return True

    #
    # Get String name and value
    #
    def GetStringObject(self, Item):
        Name = ''
        Language = ''
        Value = ''

        Name = Item.split()[1]
        LanguageList = Item.split(u'#language ')
        for IndexI in range(len(LanguageList)):
            if IndexI == 0:
                continue
            else:
                Language = LanguageList[IndexI].split()[0]
                Value = LanguageList[IndexI][LanguageList[IndexI].find(u'\"') + len(u'\"') : LanguageList[IndexI].rfind(u'\"')] #.replace(u'\r\n', u'')
                Language = GetLanguageCode(Language, self.IsCompatibleMode, self.File)
                self.AddStringToList(Name, Language, Value)

    #
    # Get include file list and load them
    #
    def GetIncludeFile(self, Item, Dir):
        FileName = Item[Item.find(u'#include ') + len(u'#include ') :Item.find(u' ', len(u'#include '))][1:-1]
        self.LoadUniFile(FileName)

    #
    # Pre-process before parse .uni file
    #
    def PreProcess(self, File):
        if not os.path.exists(File.Path) or not os.path.isfile(File.Path):
            EdkLogger.error("Unicode File Parser", FILE_NOT_FOUND, ExtraData=File.Path)

        Dir = File.Dir
        try:
            FileIn = codecs.open(File.Path, mode='rb', encoding='utf-16').readlines()
        except UnicodeError, X:
            EdkLogger.error("build", FILE_READ_FAILURE, "File read failure: %s" % str(X), ExtraData=File.Path);
        except:
            EdkLogger.error("build", FILE_OPEN_FAILURE, ExtraData=File.Path);

        Lines = []
        #
        # Use unique identifier
        #
        for Line in FileIn:
            Line = Line.strip()
            #
            # Ignore comment line and empty line
            #
            if Line == u'' or Line.startswith(u'//'):
                continue
            Line = Line.replace(u'/langdef', u'#langdef')
            Line = Line.replace(u'/string', u'#string')
            Line = Line.replace(u'/language', u'#language')
            Line = Line.replace(u'/include', u'#include')

            Line = Line.replace(UNICODE_WIDE_CHAR, WIDE_CHAR)
            Line = Line.replace(UNICODE_NARROW_CHAR, NARROW_CHAR)
            Line = Line.replace(UNICODE_NON_BREAKING_CHAR, NON_BREAKING_CHAR)

            Line = Line.replace(u'\\\\', u'\u0006')
            Line = Line.replace(u'\\r\\n', CR + LF)
            Line = Line.replace(u'\\n', CR + LF)
            Line = Line.replace(u'\\r', CR)
            Line = Line.replace(u'\\t', u'\t')
            Line = Line.replace(u'''\"''', u'''"''')
            Line = Line.replace(u'\t', u' ')
            Line = Line.replace(u'\u0006', u'\\')
            
#           if Line.find(u'\\x'):
#               hex = Line[Line.find(u'\\x') + 2 : Line.find(u'\\x') + 6]
#               hex = "u'\\u" + hex + "'"

            IncList = gIncludePattern.findall(Line)
            if len(IncList) == 1:
                Lines.extend(self.PreProcess(PathClass(str(IncList[0]), Dir)))
                continue

            Lines.append(Line)

        return Lines

    #
    # Load a .uni file
    #
    def LoadUniFile(self, File = None):
        if File == None:
            EdkLogger.error("Unicode File Parser", PARSER_ERROR, 'No unicode file is given')
        self.File = File
        #
        # Process special char in file
        #
        Lines = self.PreProcess(File)

        #
        # Get Unicode Information
        #
        for IndexI in range(len(Lines)):
            Line = Lines[IndexI]
            if (IndexI + 1) < len(Lines):
                SecondLine = Lines[IndexI + 1]
            if (IndexI + 2) < len(Lines):
                ThirdLine = Lines[IndexI + 2]

            #
            # Get Language def information
            #
            if Line.find(u'#langdef ') >= 0:
                self.GetLangDef(File, Line)
                continue

            Name = ''
            Language = ''
            Value = ''
            #
            # Get string def information format 1 as below
            #
            #     #string MY_STRING_1
            #     #language eng
            #     My first English string line 1
            #     My first English string line 2
            #     #string MY_STRING_1
            #     #language spa
            #     Mi segunda secuencia 1
            #     Mi segunda secuencia 2
            #
            if Line.find(u'#string ') >= 0 and Line.find(u'#language ') < 0 and \
                SecondLine.find(u'#string ') < 0 and SecondLine.find(u'#language ') >= 0 and \
                ThirdLine.find(u'#string ') < 0 and ThirdLine.find(u'#language ') < 0:
                Name = Line[Line.find(u'#string ') + len(u'#string ') : ].strip(' ')
                Language = SecondLine[SecondLine.find(u'#language ') + len(u'#language ') : ].strip(' ')
                for IndexJ in range(IndexI + 2, len(Lines)):
                    if Lines[IndexJ].find(u'#string ') < 0 and Lines[IndexJ].find(u'#language ') < 0:
                        Value = Value + Lines[IndexJ]
                    else:
                        IndexI = IndexJ
                        break
                # Value = Value.replace(u'\r\n', u'')
                Language = GetLanguageCode(Language, self.IsCompatibleMode, self.File)
                self.AddStringToList(Name, Language, Value)
                continue

            #
            # Get string def information format 2 as below
            #
            #     #string MY_STRING_1     #language eng     "My first English string line 1"
            #                                               "My first English string line 2"
            #                             #language spa     "Mi segunda secuencia 1"
            #                                               "Mi segunda secuencia 2"
            #     #string MY_STRING_2     #language eng     "My first English string line 1"
            #                                               "My first English string line 2"
            #     #string MY_STRING_2     #language spa     "Mi segunda secuencia 1"
            #                                               "Mi segunda secuencia 2"
            #
            if Line.find(u'#string ') >= 0 and Line.find(u'#language ') >= 0:
                StringItem = Line
                for IndexJ in range(IndexI + 1, len(Lines)):
                    if Lines[IndexJ].find(u'#string ') >= 0 and Lines[IndexJ].find(u'#language ') >= 0:
                        IndexI = IndexJ
                        break
                    elif Lines[IndexJ].find(u'#string ') < 0 and Lines[IndexJ].find(u'#language ') >= 0:
                        StringItem = StringItem + Lines[IndexJ]
                    elif Lines[IndexJ].count(u'\"') >= 2:
                        StringItem = StringItem[ : StringItem.rfind(u'\"')] + Lines[IndexJ][Lines[IndexJ].find(u'\"') + len(u'\"') : ]
                self.GetStringObject(StringItem)
                continue

    #
    # Load multiple .uni files
    #
    def LoadUniFiles(self, FileList = []):
        if len(FileList) > 0:
            if len(FileList) > 1:
                NewList = [];
                for File in FileList:
                    NewList.append (File)
                NewList.sort()
                for File in NewList:
                    self.LoadUniFile(File)
            else:
                for File in FileList:
                    self.LoadUniFile(File)

    #
    # Add a string to list
    #
    def AddStringToList(self, Name, Language, Value, Token = None, Referenced = False, UseOtherLangDef = '', Index = -1):
        if Language not in self.OrderedStringList:
            self.OrderedStringList[Language] = []

        IsAdded = False
        for Item in self.OrderedStringList[Language]:
            if Name == Item.StringName:
                IsAdded = True
                break
        if not IsAdded:
            Token = len(self.OrderedStringList[Language])
            if Index == -1:
                self.OrderedStringList[Language].append(StringDefClassObject(Name, Value, Referenced, Token, UseOtherLangDef))
            else:
                self.OrderedStringList[Language].insert(Index, StringDefClassObject(Name, Value, Referenced, Token, UseOtherLangDef))

    #
    # Set the string as referenced
    #
    def SetStringReferenced(self, Name):
        for Lang in self.OrderedStringList:
            for Item in self.OrderedStringList[Lang]:
                if Name == Item.StringName:
                    Item.Referenced = True
                    break
    #
    # Search the string in language definition by Name
    #
    def FindStringValue(self, Name, Lang):
        for Item in self.OrderedStringList[Lang]:
            if Item.StringName == Name:
                return Item

        return None

    #
    # Search the string in language definition by Token
    #
    def FindByToken(self, Token, Lang):
        for Item in self.OrderedStringList[Lang]:
            if Item.Token == Token:
                return Item

        return None

    #
    # Re-order strings and re-generate tokens
    #
    def ReToken(self):
        #
        # Search each string to find if it is defined for each language
        # Use secondary language value to replace if missing in any one language
        #
        for IndexI in range(0, len(self.LanguageDef)):
            LangKey = self.LanguageDef[IndexI][0]
            for Item in self.OrderedStringList[LangKey]:
                Name = Item.StringName
                Value = Item.StringValue[0:-1]
                Referenced = Item.Referenced
                Index = self.OrderedStringList[LangKey].index(Item)
                for IndexJ in range(0, len(self.LanguageDef)):
                    LangFind = self.LanguageDef[IndexJ][0]
                    if self.FindStringValue(Name, LangFind) == None:
                        EdkLogger.debug(EdkLogger.DEBUG_5, Name)
                        Token = len(self.OrderedStringList[LangFind])
                        self.AddStringToList(Name, LangFind, Value, Token, Referenced, LangKey, Index)

        #
        # Retoken
        #
        # First re-token the first language
        LangName = self.LanguageDef[0][0]
        ReferencedStringList = []
        NotReferencedStringList = []
        Token = 0
        for Item in self.OrderedStringList[LangName]:
            if Item.Referenced == True:
                Item.Token = Token
                ReferencedStringList.append(Item)
                Token = Token + 1
            else:
                NotReferencedStringList.append(Item)
        self.OrderedStringList[LangName] = ReferencedStringList
        for Index in range(len(NotReferencedStringList)):
            NotReferencedStringList[Index].Token = Token + Index
            self.OrderedStringList[LangName].append(NotReferencedStringList[Index])

        #
        # Adjust the orders of other languages
        #
        for IndexOfLanguage in range(1, len(self.LanguageDef)):
            for OrderedString in self.OrderedStringList[LangName]:
                for UnOrderedString in self.OrderedStringList[self.LanguageDef[IndexOfLanguage][0]]:
                    if OrderedString.StringName == UnOrderedString.StringName:
                        UnOrderedString.Token = OrderedString.Token
                        break

    #
    # Show the instance itself
    #
    def ShowMe(self):
        print self.LanguageDef
        #print self.OrderedStringList
        for Item in self.OrderedStringList:
            print Item
            for Member in self.OrderedStringList[Item]:
                print str(Member)

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)
    a = UniFileClassObject(['C:\\Edk\\Strings.uni', 'C:\\Edk\\Strings2.uni'])
    a.ReToken()
    a.ShowMe()
