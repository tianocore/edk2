## @file
# Collect all defined strings in multiple uni files.
#
# Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
"""
Collect all defined strings in multiple uni files
"""
from __future__ import print_function

##
# Import Modules
#
import os, codecs, re
import shlex
from Logger import ToolError
from Logger import Log as EdkLogger
from Logger import StringTable as ST
from Library.StringUtils import GetLineNo
from Library.Misc import PathClass
from Library.Misc import GetCharIndexOutStr
from Library import DataType as DT
from Library.ParserValidate import CheckUTF16FileHeader

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

gLANG_CONV_TABLE = {'eng':'en', 'fra':'fr', \
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

## Convert special unicode characters
#
# Convert special characters to (c), (r) and (tm).
#
# @param Uni:    The python unicode string
#
# @retval NewUni:  The converted unicode string
#
def ConvertSpecialUnicodes(Uni):
    OldUni = NewUni = Uni
    NewUni = NewUni.replace(u'\u00A9', '(c)')
    NewUni = NewUni.replace(u'\u00AE', '(r)')
    NewUni = NewUni.replace(u'\u2122', '(tm)')
    if OldUni == NewUni:
        NewUni = OldUni
    return NewUni

## GetLanguageCode1766
#
# Check the language code read from .UNI file and convert RFC 4646 codes to RFC 1766 codes
# RFC 1766 language codes supported in compatibility mode
# RFC 4646 language codes supported in native mode
#
# @param LangName:   Language codes read from .UNI file
#
# @retval LangName:  Valid language code in RFC 1766 format or None
#
def GetLanguageCode1766(LangName, File=None):
    return LangName

    length = len(LangName)
    if length == 2:
        if LangName.isalpha():
            for Key in gLANG_CONV_TABLE.keys():
                if gLANG_CONV_TABLE.get(Key) == LangName.lower():
                    return Key
    elif length == 3:
        if LangName.isalpha() and gLANG_CONV_TABLE.get(LangName.lower()):
            return LangName
        else:
            EdkLogger.Error("Unicode File Parser",
                             ToolError.FORMAT_INVALID,
                             "Invalid RFC 1766 language code : %s" % LangName,
                             File)
    elif length == 5:
        if LangName[0:2].isalpha() and LangName[2] == '-':
            for Key in gLANG_CONV_TABLE.keys():
                if gLANG_CONV_TABLE.get(Key) == LangName[0:2].lower():
                    return Key
    elif length >= 6:
        if LangName[0:2].isalpha() and LangName[2] == '-':
            for Key in gLANG_CONV_TABLE.keys():
                if gLANG_CONV_TABLE.get(Key) == LangName[0:2].lower():
                    return Key
        if LangName[0:3].isalpha() and gLANG_CONV_TABLE.get(LangName.lower()) is None and LangName[3] == '-':
            for Key in gLANG_CONV_TABLE.keys():
                if Key == LangName[0:3].lower():
                    return Key

    EdkLogger.Error("Unicode File Parser",
                             ToolError.FORMAT_INVALID,
                             "Invalid RFC 4646 language code : %s" % LangName,
                             File)

## GetLanguageCode
#
# Check the language code read from .UNI file and convert RFC 1766 codes to RFC 4646 codes if appropriate
# RFC 1766 language codes supported in compatibility mode
# RFC 4646 language codes supported in native mode
#
# @param LangName:   Language codes read from .UNI file
#
# @retval LangName:  Valid lanugage code in RFC 4646 format or None
#
def GetLanguageCode(LangName, IsCompatibleMode, File):
    length = len(LangName)
    if IsCompatibleMode:
        if length == 3 and LangName.isalpha():
            TempLangName = gLANG_CONV_TABLE.get(LangName.lower())
            if TempLangName is not None:
                return TempLangName
            return LangName
        else:
            EdkLogger.Error("Unicode File Parser",
                             ToolError.FORMAT_INVALID,
                             "Invalid RFC 1766 language code : %s" % LangName,
                             File)
    if (LangName[0] == 'X' or LangName[0] == 'x') and LangName[1] == '-':
        return LangName
    if length == 2:
        if LangName.isalpha():
            return LangName
    elif length == 3:
        if LangName.isalpha() and gLANG_CONV_TABLE.get(LangName.lower()) is None:
            return LangName
    elif length == 5:
        if LangName[0:2].isalpha() and LangName[2] == '-':
            return LangName
    elif length >= 6:
        if LangName[0:2].isalpha() and LangName[2] == '-':
            return LangName
        if LangName[0:3].isalpha() and gLANG_CONV_TABLE.get(LangName.lower()) is None and LangName[3] == '-':
            return LangName

    EdkLogger.Error("Unicode File Parser",
                             ToolError.FORMAT_INVALID,
                             "Invalid RFC 4646 language code : %s" % LangName,
                             File)

## FormatUniEntry
#
# Formatted the entry in Uni file.
#
# @param StrTokenName    StrTokenName.
# @param TokenValueList  A list need to be processed.
# @param ContainerFile   ContainerFile.
#
# @return formatted entry
def FormatUniEntry(StrTokenName, TokenValueList, ContainerFile):
    SubContent = ''
    PreFormatLength = 40
    if len(StrTokenName) > PreFormatLength:
        PreFormatLength = len(StrTokenName) + 1
    for (Lang, Value) in TokenValueList:
        if not Value or Lang == DT.TAB_LANGUAGE_EN_X:
            continue
        if Lang == '':
            Lang = DT.TAB_LANGUAGE_EN_US
        if Lang == 'eng':
            Lang = DT.TAB_LANGUAGE_EN_US
        elif len(Lang.split('-')[0]) == 3:
            Lang = GetLanguageCode(Lang.split('-')[0], True, ContainerFile)
        else:
            Lang = GetLanguageCode(Lang, False, ContainerFile)
        ValueList = Value.split('\n')
        SubValueContent = ''
        for SubValue in ValueList:
            if SubValue.strip():
                SubValueContent += \
                ' ' * (PreFormatLength + len('#language en-US ')) + '\"%s\\n\"' % SubValue.strip() + '\r\n'
        SubValueContent = SubValueContent[(PreFormatLength + len('#language en-US ')):SubValueContent.rfind('\\n')] \
        + '\"' + '\r\n'
        SubContent += ' '*PreFormatLength + '#language %-5s ' % Lang + SubValueContent
    if SubContent:
        SubContent = StrTokenName + ' '*(PreFormatLength - len(StrTokenName)) + SubContent[PreFormatLength:]
    return SubContent


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

        if Name is not None:
            self.StringName = Name
            self.StringNameByteList = UniToHexList(Name)
        if Value is not None:
            self.StringValue = Value
            self.StringValueByteList = UniToHexList(self.StringValue)
            self.Length = len(self.StringValueByteList)
        if Token is not None:
            self.Token = Token

    def __str__(self):
        return repr(self.StringName) + ' ' + \
               repr(self.Token) + ' ' + \
               repr(self.Referenced) + ' ' + \
               repr(self.StringValue) + ' ' + \
               repr(self.UseOtherLangDef)

    def UpdateValue(self, Value = None):
        if Value is not None:
            if self.StringValue:
                self.StringValue = self.StringValue + '\r\n' + Value
            else:
                self.StringValue = Value
            self.StringValueByteList = UniToHexList(self.StringValue)
            self.Length = len(self.StringValueByteList)

## UniFileClassObject
#
# A structure for .uni file definition
#
class UniFileClassObject(object):
    def __init__(self, FileList = None, IsCompatibleMode = False, IncludePathList = None):
        self.FileList = FileList
        self.File = None
        self.IncFileList = FileList
        self.UniFileHeader = ''
        self.Token = 2
        self.LanguageDef = []                   #[ [u'LanguageIdentifier', u'PrintableName'], ... ]
        self.OrderedStringList = {}             #{ u'LanguageIdentifier' : [StringDefClassObject]  }
        self.OrderedStringDict = {}             #{ u'LanguageIdentifier' : {StringName:(IndexInList)}  }
        self.OrderedStringListByToken = {}      #{ u'LanguageIdentifier' : {Token: StringDefClassObject} }
        self.IsCompatibleMode = IsCompatibleMode
        if not IncludePathList:
            self.IncludePathList = []
        else:
            self.IncludePathList = IncludePathList
        if len(self.FileList) > 0:
            self.LoadUniFiles(FileList)

    #
    # Get Language definition
    #
    def GetLangDef(self, File, Line):
        Lang = shlex.split(Line.split(u"//")[0])
        if len(Lang) != 3:
            try:
                FileIn = codecs.open(File.Path, mode='rb', encoding='utf_8').readlines()
            except UnicodeError as Xstr:
                FileIn = codecs.open(File.Path, mode='rb', encoding='utf_16').readlines()
            except UnicodeError as Xstr:
                FileIn = codecs.open(File.Path, mode='rb', encoding='utf_16_le').readlines()
            except:
                EdkLogger.Error("Unicode File Parser",
                                ToolError.FILE_OPEN_FAILURE,
                                "File read failure: %s" % str(Xstr),
                                ExtraData=File)
            LineNo = GetLineNo(FileIn, Line, False)
            EdkLogger.Error("Unicode File Parser",
                             ToolError.PARSER_ERROR,
                             "Wrong language definition",
                             ExtraData="""%s\n\t*Correct format is like '#langdef en-US "English"'""" % Line,
                             File = File, Line = LineNo)
        else:
            LangName = GetLanguageCode(Lang[1], self.IsCompatibleMode, self.File)
            LangPrintName = Lang[2]

        IsLangInDef = False
        for Item in self.LanguageDef:
            if Item[0] == LangName:
                IsLangInDef = True
                break

        if not IsLangInDef:
            self.LanguageDef.append([LangName, LangPrintName])

        #
        # Add language string
        #
        self.AddStringToList(u'$LANGUAGE_NAME', LangName, LangName, 0, True, Index=0)
        self.AddStringToList(u'$PRINTABLE_LANGUAGE_NAME', LangName, LangPrintName, 1, True, Index=1)

        if not IsLangInDef:
            #
            # The found STRING tokens will be added into new language string list
            # so that the unique STRING identifier is reserved for all languages in the package list.
            #
            FirstLangName = self.LanguageDef[0][0]
            if LangName != FirstLangName:
                for Index in range (2, len (self.OrderedStringList[FirstLangName])):
                    Item = self.OrderedStringList[FirstLangName][Index]
                    if Item.UseOtherLangDef != '':
                        OtherLang = Item.UseOtherLangDef
                    else:
                        OtherLang = FirstLangName
                    self.OrderedStringList[LangName].append (StringDefClassObject(Item.StringName,
                                                                                  '',
                                                                                  Item.Referenced,
                                                                                  Item.Token,
                                                                                  OtherLang))
                    self.OrderedStringDict[LangName][Item.StringName] = len(self.OrderedStringList[LangName]) - 1
        return True

    #
    # Get String name and value
    #
    def GetStringObject(self, Item):
        Language = ''
        Value = ''

        Name = Item.split()[1]
        # Check the string name is the upper character
        if Name != '':
            MatchString = re.match('[A-Z0-9_]+', Name, re.UNICODE)
            if MatchString is None or MatchString.end(0) != len(Name):
                EdkLogger.Error("Unicode File Parser",
                             ToolError.FORMAT_INVALID,
                             'The string token name %s in UNI file %s must be upper case character.' %(Name, self.File))
        LanguageList = Item.split(u'#language ')
        for IndexI in range(len(LanguageList)):
            if IndexI == 0:
                continue
            else:
                Language = LanguageList[IndexI].split()[0]
                #.replace(u'\r\n', u'')
                Value = \
                LanguageList[IndexI][LanguageList[IndexI].find(u'\"') + len(u'\"') : LanguageList[IndexI].rfind(u'\"')]
                Language = GetLanguageCode(Language, self.IsCompatibleMode, self.File)
                self.AddStringToList(Name, Language, Value)

    #
    # Get include file list and load them
    #
    def GetIncludeFile(self, Item, Dir = None):
        if Dir:
            pass
        FileName = Item[Item.find(u'!include ') + len(u'!include ') :Item.find(u' ', len(u'!include '))][1:-1]
        self.LoadUniFile(FileName)

    #
    # Pre-process before parse .uni file
    #
    def PreProcess(self, File, IsIncludeFile=False):
        if not os.path.exists(File.Path) or not os.path.isfile(File.Path):
            EdkLogger.Error("Unicode File Parser",
                             ToolError.FILE_NOT_FOUND,
                             ExtraData=File.Path)

        #
        # Check file header of the Uni file
        #
#         if not CheckUTF16FileHeader(File.Path):
#             EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
#                             ExtraData='The file %s is either invalid UTF-16LE or it is missing the BOM.' % File.Path)

        try:
            FileIn = codecs.open(File.Path, mode='rb', encoding='utf_8').readlines()
        except UnicodeError as Xstr:
            FileIn = codecs.open(File.Path, mode='rb', encoding='utf_16').readlines()
        except UnicodeError:
            FileIn = codecs.open(File.Path, mode='rb', encoding='utf_16_le').readlines()
        except:
            EdkLogger.Error("Unicode File Parser", ToolError.FILE_OPEN_FAILURE, ExtraData=File.Path)


        #
        # get the file header
        #
        Lines = []
        HeaderStart = False
        HeaderEnd = False
        if not self.UniFileHeader:
            FirstGenHeader = True
        else:
            FirstGenHeader = False
        for Line in FileIn:
            Line = Line.strip()
            if Line == u'':
                continue
            if Line.startswith(DT.TAB_COMMENT_EDK1_SPLIT) and (Line.find(DT.TAB_HEADER_COMMENT) > -1) \
                and not HeaderEnd and not HeaderStart:
                HeaderStart = True
            if not Line.startswith(DT.TAB_COMMENT_EDK1_SPLIT) and HeaderStart and not HeaderEnd:
                HeaderEnd = True
            if Line.startswith(DT.TAB_COMMENT_EDK1_SPLIT) and HeaderStart and not HeaderEnd and FirstGenHeader:
                self.UniFileHeader += Line + '\r\n'
                continue

        #
        # Use unique identifier
        #
        FindFlag = -1
        LineCount = 0
        MultiLineFeedExits = False
        #
        # 0: initial value
        # 1: single String entry exist
        # 2: line feed exist under the some single String entry
        #
        StringEntryExistsFlag = 0
        for Line in FileIn:
            Line = FileIn[LineCount]
            LineCount += 1
            Line = Line.strip()
            #
            # Ignore comment line and empty line
            #
            if Line == u'' or Line.startswith(u'//'):
                #
                # Change the single line String entry flag status
                #
                if StringEntryExistsFlag == 1:
                    StringEntryExistsFlag = 2
                #
                # If the '#string' line and the '#language' line are not in the same line,
                # there should be only one line feed character between them
                #
                if MultiLineFeedExits:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)
                continue

            MultiLineFeedExits = False
            #
            # Process comment embedded in string define lines
            #
            FindFlag = Line.find(u'//')
            if FindFlag != -1 and Line.find(u'//') < Line.find(u'"'):
                Line = Line.replace(Line[FindFlag:], u' ')
                if FileIn[LineCount].strip().startswith('#language'):
                    Line = Line + FileIn[LineCount]
                    FileIn[LineCount-1] = Line
                    FileIn[LineCount] = '\r\n'
                    LineCount -= 1
                    for Index in range (LineCount + 1, len (FileIn) - 1):
                        if (Index == len(FileIn) -1):
                            FileIn[Index] = '\r\n'
                        else:
                            FileIn[Index] = FileIn[Index + 1]
                    continue
            CommIndex = GetCharIndexOutStr(u'/', Line)
            if CommIndex > -1:
                if (len(Line) - 1) > CommIndex:
                    if Line[CommIndex+1] == u'/':
                        Line = Line[:CommIndex].strip()
                    else:
                        EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)
                else:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)

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

            #
            # Check if single line has correct '"'
            #
            if Line.startswith(u'#string') and Line.find(u'#language') > -1 and Line.find('"') > Line.find(u'#language'):
                if not Line.endswith('"'):
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                    ExtraData='''The line %s misses '"' at the end of it in file %s'''
                                                 % (LineCount, File.Path))

            #
            # Between Name entry and Language entry can not contain line feed
            #
            if Line.startswith(u'#string') and Line.find(u'#language') == -1:
                MultiLineFeedExits = True

            if Line.startswith(u'#string') and Line.find(u'#language') > 0 and Line.find(u'"') < 0:
                MultiLineFeedExits = True

            #
            # Between Language entry and String entry can not contain line feed
            #
            if Line.startswith(u'#language') and len(Line.split()) == 2:
                MultiLineFeedExits = True

            #
            # Check the situation that there only has one '"' for the language entry
            #
            if Line.startswith(u'#string') and Line.find(u'#language') > 0 and Line.count(u'"') == 1:
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                ExtraData='''The line %s misses '"' at the end of it in file %s'''
                                % (LineCount, File.Path))

            #
            # Check the situation that there has more than 2 '"' for the language entry
            #
            if Line.startswith(u'#string') and Line.find(u'#language') > 0 and Line.replace(u'\\"', '').count(u'"') > 2:
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                ExtraData='''The line %s has more than 2 '"' for language entry in file %s'''
                                % (LineCount, File.Path))

            #
            # Between two String entry, can not contain line feed
            #
            if Line.startswith(u'"'):
                if StringEntryExistsFlag == 2:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                    Message=ST.ERR_UNIPARSE_LINEFEED_UP_EXIST % Line, ExtraData=File.Path)

                StringEntryExistsFlag = 1
                if not Line.endswith('"'):
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                    ExtraData='''The line %s misses '"' at the end of it in file %s'''
                                              % (LineCount, File.Path))

                #
                # Check the situation that there has more than 2 '"' for the language entry
                #
                if Line.strip() and Line.replace(u'\\"', '').count(u'"') > 2:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                    ExtraData='''The line %s has more than 2 '"' for language entry in file %s'''
                                    % (LineCount, File.Path))

            elif Line.startswith(u'#language'):
                if StringEntryExistsFlag == 2:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                    Message=ST.ERR_UNI_MISS_STRING_ENTRY % Line, ExtraData=File.Path)
                StringEntryExistsFlag = 0
            else:
                StringEntryExistsFlag = 0

            Lines.append(Line)

        #
        # Convert string def format as below
        #
        #     #string MY_STRING_1
        #     #language eng
        #     "My first English string line 1"
        #     "My first English string line 2"
        #     #string MY_STRING_1
        #     #language spa
        #     "Mi segunda secuencia 1"
        #     "Mi segunda secuencia 2"
        #

        if not IsIncludeFile and not Lines:
            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                Message=ST.ERR_UNIPARSE_NO_SECTION_EXIST, \
                ExtraData=File.Path)

        NewLines = []
        StrName = u''
        ExistStrNameList = []
        for Line in Lines:
            if StrName and not StrName.split()[1].startswith(DT.TAB_STR_TOKENCNAME + DT.TAB_UNDERLINE_SPLIT):
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_STRNAME_FORMAT_ERROR % StrName.split()[1], \
                                ExtraData=File.Path)

            if StrName and len(StrName.split()[1].split(DT.TAB_UNDERLINE_SPLIT)) == 4:
                StringTokenList = StrName.split()[1].split(DT.TAB_UNDERLINE_SPLIT)
                if (StringTokenList[3].upper() in [DT.TAB_STR_TOKENPROMPT, DT.TAB_STR_TOKENHELP] and \
                    StringTokenList[3] not in [DT.TAB_STR_TOKENPROMPT, DT.TAB_STR_TOKENHELP]) or \
                    (StringTokenList[2].upper() == DT.TAB_STR_TOKENERR and StringTokenList[2] != DT.TAB_STR_TOKENERR):
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_STRTOKEN_FORMAT_ERROR % StrName.split()[1], \
                                ExtraData=File.Path)

            if Line.count(u'#language') > 1:
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_SEP_LANGENTRY_LINE % Line, \
                                ExtraData=File.Path)

            if Line.startswith(u'//'):
                continue
            elif Line.startswith(u'#langdef'):
                if len(Line.split()) == 2:
                    NewLines.append(Line)
                    continue
                elif len(Line.split()) > 2 and Line.find(u'"') > 0:
                    NewLines.append(Line[:Line.find(u'"')].strip())
                    NewLines.append(Line[Line.find(u'"'):])
                else:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)
            elif Line.startswith(u'#string'):
                if len(Line.split()) == 2:
                    StrName = Line
                    if StrName:
                        if StrName.split()[1] not in ExistStrNameList:
                            ExistStrNameList.append(StrName.split()[1].strip())
                        elif StrName.split()[1] in [DT.TAB_INF_ABSTRACT, DT.TAB_INF_DESCRIPTION, \
                                                    DT.TAB_INF_BINARY_ABSTRACT, DT.TAB_INF_BINARY_DESCRIPTION, \
                                                    DT.TAB_DEC_PACKAGE_ABSTRACT, DT.TAB_DEC_PACKAGE_DESCRIPTION, \
                                                    DT.TAB_DEC_BINARY_ABSTRACT, DT.TAB_DEC_BINARY_DESCRIPTION]:
                            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                            Message=ST.ERR_UNIPARSE_MULTI_ENTRY_EXIST % StrName.split()[1], \
                                            ExtraData=File.Path)
                    continue
                elif len(Line.split()) == 4 and Line.find(u'#language') > 0:
                    if Line[Line.find(u'#language')-1] != ' ' or \
                       Line[Line.find(u'#language')+len(u'#language')] != u' ':
                        EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)

                    if Line.find(u'"') > 0:
                        EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)

                    StrName = Line.split()[0] + u' ' + Line.split()[1]
                    if StrName:
                        if StrName.split()[1] not in ExistStrNameList:
                            ExistStrNameList.append(StrName.split()[1].strip())
                        elif StrName.split()[1] in [DT.TAB_INF_ABSTRACT, DT.TAB_INF_DESCRIPTION, \
                                                    DT.TAB_INF_BINARY_ABSTRACT, DT.TAB_INF_BINARY_DESCRIPTION, \
                                                    DT.TAB_DEC_PACKAGE_ABSTRACT, DT.TAB_DEC_PACKAGE_DESCRIPTION, \
                                                    DT.TAB_DEC_BINARY_ABSTRACT, DT.TAB_DEC_BINARY_DESCRIPTION]:
                            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                            Message=ST.ERR_UNIPARSE_MULTI_ENTRY_EXIST % StrName.split()[1], \
                                            ExtraData=File.Path)
                    if IsIncludeFile:
                        if StrName not in NewLines:
                            NewLines.append((Line[:Line.find(u'#language')]).strip())
                    else:
                        NewLines.append((Line[:Line.find(u'#language')]).strip())
                    NewLines.append((Line[Line.find(u'#language'):]).strip())
                elif len(Line.split()) > 4 and Line.find(u'#language') > 0 and Line.find(u'"') > 0:
                    if Line[Line.find(u'#language')-1] != u' ' or \
                       Line[Line.find(u'#language')+len(u'#language')] != u' ':
                        EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)

                    if Line[Line.find(u'"')-1] != u' ':
                        EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)

                    StrName = Line.split()[0] + u' ' + Line.split()[1]
                    if StrName:
                        if StrName.split()[1] not in ExistStrNameList:
                            ExistStrNameList.append(StrName.split()[1].strip())
                        elif StrName.split()[1] in [DT.TAB_INF_ABSTRACT, DT.TAB_INF_DESCRIPTION, \
                                                    DT.TAB_INF_BINARY_ABSTRACT, DT.TAB_INF_BINARY_DESCRIPTION, \
                                                    DT.TAB_DEC_PACKAGE_ABSTRACT, DT.TAB_DEC_PACKAGE_DESCRIPTION, \
                                                    DT.TAB_DEC_BINARY_ABSTRACT, DT.TAB_DEC_BINARY_DESCRIPTION]:
                            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                            Message=ST.ERR_UNIPARSE_MULTI_ENTRY_EXIST % StrName.split()[1], \
                                            ExtraData=File.Path)
                    if IsIncludeFile:
                        if StrName not in NewLines:
                            NewLines.append((Line[:Line.find(u'#language')]).strip())
                    else:
                        NewLines.append((Line[:Line.find(u'#language')]).strip())
                    NewLines.append((Line[Line.find(u'#language'):Line.find(u'"')]).strip())
                    NewLines.append((Line[Line.find(u'"'):]).strip())
                else:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)
            elif Line.startswith(u'#language'):
                if len(Line.split()) == 2:
                    if IsIncludeFile:
                        if StrName not in NewLines:
                            NewLines.append(StrName)
                    else:
                        NewLines.append(StrName)
                    NewLines.append(Line)
                elif len(Line.split()) > 2 and Line.find(u'"') > 0:
                    if IsIncludeFile:
                        if StrName not in NewLines:
                            NewLines.append(StrName)
                    else:
                        NewLines.append(StrName)
                    NewLines.append((Line[:Line.find(u'"')]).strip())
                    NewLines.append((Line[Line.find(u'"'):]).strip())
                else:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)
            elif Line.startswith(u'"'):
                if u'#string' in Line  or u'#language' in Line:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)
                NewLines.append(Line)
            else:
                print(Line)
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, ExtraData=File.Path)

        if StrName and not StrName.split()[1].startswith(u'STR_'):
            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_STRNAME_FORMAT_ERROR % StrName.split()[1], \
                                ExtraData=File.Path)

        if StrName and not NewLines:
            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                            Message=ST.ERR_UNI_MISS_LANGENTRY % StrName, \
                            ExtraData=File.Path)

        #
        # Check Abstract, Description, BinaryAbstract and BinaryDescription order,
        # should be Abstract, Description, BinaryAbstract, BinaryDescription
        AbstractPosition = -1
        DescriptionPosition = -1
        BinaryAbstractPosition = -1
        BinaryDescriptionPosition = -1
        for StrName in ExistStrNameList:
            if DT.TAB_HEADER_ABSTRACT.upper() in StrName:
                if 'BINARY' in StrName:
                    BinaryAbstractPosition = ExistStrNameList.index(StrName)
                else:
                    AbstractPosition = ExistStrNameList.index(StrName)
            if DT.TAB_HEADER_DESCRIPTION.upper() in StrName:
                if 'BINARY' in StrName:
                    BinaryDescriptionPosition = ExistStrNameList.index(StrName)
                else:
                    DescriptionPosition = ExistStrNameList.index(StrName)

        OrderList = sorted([AbstractPosition, DescriptionPosition])
        BinaryOrderList = sorted([BinaryAbstractPosition, BinaryDescriptionPosition])
        Min = OrderList[0]
        Max = OrderList[1]
        BinaryMin = BinaryOrderList[0]
        BinaryMax = BinaryOrderList[1]
        if BinaryDescriptionPosition > -1:
            if not(BinaryDescriptionPosition == BinaryMax and BinaryAbstractPosition == BinaryMin and \
                   BinaryMax > Max):
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_ENTRY_ORDER_WRONG, \
                                ExtraData=File.Path)
        elif BinaryAbstractPosition > -1:
            if not(BinaryAbstractPosition > Max):
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_ENTRY_ORDER_WRONG, \
                                ExtraData=File.Path)

        if  DescriptionPosition > -1:
            if not(DescriptionPosition == Max and AbstractPosition == Min and \
                   DescriptionPosition > AbstractPosition):
                EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID, \
                                Message=ST.ERR_UNIPARSE_ENTRY_ORDER_WRONG, \
                                ExtraData=File.Path)

        if not self.UniFileHeader:
            EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                            Message = ST.ERR_NO_SOURCE_HEADER,
                            ExtraData=File.Path)

        return NewLines

    #
    # Load a .uni file
    #
    def LoadUniFile(self, File = None):
        if File is None:
            EdkLogger.Error("Unicode File Parser",
                            ToolError.PARSER_ERROR,
                            Message='No unicode file is given',
                            ExtraData=File.Path)

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
                self.GetLangDef(File, Line + u' ' + SecondLine)
                continue

            Name = ''
            Language = ''
            Value = ''
            CombineToken = False
            #
            # Get string def information format as below
            #
            #     #string MY_STRING_1
            #     #language eng
            #     "My first English string line 1"
            #     "My first English string line 2"
            #     #string MY_STRING_1
            #     #language spa
            #     "Mi segunda secuencia 1"
            #     "Mi segunda secuencia 2"
            #
            if Line.find(u'#string ') >= 0 and Line.find(u'#language ') < 0 and \
                SecondLine.find(u'#string ') < 0 and SecondLine.find(u'#language ') >= 0 and \
                ThirdLine.find(u'#string ') < 0 and ThirdLine.find(u'#language ') < 0:
                if Line.find('"') > 0 or SecondLine.find('"') > 0:
                    EdkLogger.Error("Unicode File Parser", ToolError.FORMAT_INVALID,
                                Message=ST.ERR_UNIPARSE_DBLQUOTE_UNMATCHED,
                                ExtraData=File.Path)

                Name = Line[Line.find(u'#string ') + len(u'#string ') : ].strip(' ')
                Language = SecondLine[SecondLine.find(u'#language ') + len(u'#language ') : ].strip(' ')
                for IndexJ in range(IndexI + 2, len(Lines)):
                    if Lines[IndexJ].find(u'#string ') < 0 and Lines[IndexJ].find(u'#language ') < 0 and \
                    Lines[IndexJ].strip().startswith(u'"') and Lines[IndexJ].strip().endswith(u'"'):
                        if Lines[IndexJ][-2] == ' ':
                            CombineToken = True
                        if CombineToken:
                            if Lines[IndexJ].strip()[1:-1].strip():
                                Value = Value + Lines[IndexJ].strip()[1:-1].rstrip() + ' '
                            else:
                                Value = Value + Lines[IndexJ].strip()[1:-1]
                            CombineToken = False
                        else:
                            Value = Value + Lines[IndexJ].strip()[1:-1] + '\r\n'
                    else:
                        IndexI = IndexJ
                        break
                if Value.endswith('\r\n'):
                    Value = Value[: Value.rfind('\r\n')]
                Language = GetLanguageCode(Language, self.IsCompatibleMode, self.File)
                self.AddStringToList(Name, Language, Value)
                continue

    #
    # Load multiple .uni files
    #
    def LoadUniFiles(self, FileList):
        if len(FileList) > 0:
            for File in FileList:
                FilePath = File.Path.strip()
                if FilePath.endswith('.uni') or FilePath.endswith('.UNI') or FilePath.endswith('.Uni'):
                    self.LoadUniFile(File)

    #
    # Add a string to list
    #
    def AddStringToList(self, Name, Language, Value, Token = 0, Referenced = False, UseOtherLangDef = '', Index = -1):
        for LangNameItem in self.LanguageDef:
            if Language == LangNameItem[0]:
                break

        if Language not in self.OrderedStringList:
            self.OrderedStringList[Language] = []
            self.OrderedStringDict[Language] = {}

        IsAdded = True
        if Name in self.OrderedStringDict[Language]:
            IsAdded = False
            if Value is not None:
                ItemIndexInList = self.OrderedStringDict[Language][Name]
                Item = self.OrderedStringList[Language][ItemIndexInList]
                Item.UpdateValue(Value)
                Item.UseOtherLangDef = ''

        if IsAdded:
            Token = len(self.OrderedStringList[Language])
            if Index == -1:
                self.OrderedStringList[Language].append(StringDefClassObject(Name,
                                                                             Value,
                                                                             Referenced,
                                                                             Token,
                                                                             UseOtherLangDef))
                self.OrderedStringDict[Language][Name] = Token
                for LangName in self.LanguageDef:
                    #
                    # New STRING token will be added into all language string lists.
                    # so that the unique STRING identifier is reserved for all languages in the package list.
                    #
                    if LangName[0] != Language:
                        if UseOtherLangDef != '':
                            OtherLangDef = UseOtherLangDef
                        else:
                            OtherLangDef = Language
                        self.OrderedStringList[LangName[0]].append(StringDefClassObject(Name,
                                                                                        '',
                                                                                        Referenced,
                                                                                        Token,
                                                                                        OtherLangDef))
                        self.OrderedStringDict[LangName[0]][Name] = len(self.OrderedStringList[LangName[0]]) - 1
            else:
                self.OrderedStringList[Language].insert(Index, StringDefClassObject(Name,
                                                                                    Value,
                                                                                    Referenced,
                                                                                    Token,
                                                                                    UseOtherLangDef))
                self.OrderedStringDict[Language][Name] = Index

    #
    # Set the string as referenced
    #
    def SetStringReferenced(self, Name):
        #
        # String stoken are added in the same order in all language string lists.
        # So, only update the status of string stoken in first language string list.
        #
        Lang = self.LanguageDef[0][0]
        if Name in self.OrderedStringDict[Lang]:
            ItemIndexInList = self.OrderedStringDict[Lang][Name]
            Item = self.OrderedStringList[Lang][ItemIndexInList]
            Item.Referenced = True

    #
    # Search the string in language definition by Name
    #
    def FindStringValue(self, Name, Lang):
        if Name in self.OrderedStringDict[Lang]:
            ItemIndexInList = self.OrderedStringDict[Lang][Name]
            return self.OrderedStringList[Lang][ItemIndexInList]

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
        if len(self.LanguageDef) == 0:
            return None
        #
        # Retoken all language strings according to the status of string stoken in the first language string.
        #
        FirstLangName = self.LanguageDef[0][0]

        # Convert the OrderedStringList to be OrderedStringListByToken in order to faciliate future search by token
        for LangNameItem in self.LanguageDef:
            self.OrderedStringListByToken[LangNameItem[0]] = {}

        #
        # Use small token for all referred string stoken.
        #
        RefToken = 0
        for Index in range (0, len (self.OrderedStringList[FirstLangName])):
            FirstLangItem = self.OrderedStringList[FirstLangName][Index]
            if FirstLangItem.Referenced == True:
                for LangNameItem in self.LanguageDef:
                    LangName = LangNameItem[0]
                    OtherLangItem = self.OrderedStringList[LangName][Index]
                    OtherLangItem.Referenced = True
                    OtherLangItem.Token = RefToken
                    self.OrderedStringListByToken[LangName][OtherLangItem.Token] = OtherLangItem
                RefToken = RefToken + 1

        #
        # Use big token for all unreferred string stoken.
        #
        UnRefToken = 0
        for Index in range (0, len (self.OrderedStringList[FirstLangName])):
            FirstLangItem = self.OrderedStringList[FirstLangName][Index]
            if FirstLangItem.Referenced == False:
                for LangNameItem in self.LanguageDef:
                    LangName = LangNameItem[0]
                    OtherLangItem = self.OrderedStringList[LangName][Index]
                    OtherLangItem.Token = RefToken + UnRefToken
                    self.OrderedStringListByToken[LangName][OtherLangItem.Token] = OtherLangItem
                UnRefToken = UnRefToken + 1

    #
    # Show the instance itself
    #
    def ShowMe(self):
        print(self.LanguageDef)
        #print self.OrderedStringList
        for Item in self.OrderedStringList:
            print(Item)
            for Member in self.OrderedStringList[Item]:
                print(str(Member))

    #
    # Read content from '!include' UNI file
    #
    def ReadIncludeUNIfile(self, FilaPath):
        if self.File:
            pass

        if not os.path.exists(FilaPath) or not os.path.isfile(FilaPath):
            EdkLogger.Error("Unicode File Parser",
                             ToolError.FILE_NOT_FOUND,
                             ExtraData=FilaPath)
        try:
            FileIn = codecs.open(FilaPath, mode='rb', encoding='utf_8').readlines()
        except UnicodeError as Xstr:
            FileIn = codecs.open(FilaPath, mode='rb', encoding='utf_16').readlines()
        except UnicodeError:
            FileIn = codecs.open(FilaPath, mode='rb', encoding='utf_16_le').readlines()
        except:
            EdkLogger.Error("Unicode File Parser", ToolError.FILE_OPEN_FAILURE, ExtraData=FilaPath)
        return FileIn

