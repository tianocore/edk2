## @file
# This file is used to parse a PCD file of .PKG file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
PcdXml
'''

##
# Import Modules
#

from Library.Xml.XmlRoutines import XmlElement
from Library.Xml.XmlRoutines import XmlAttribute
from Library.Xml.XmlRoutines import XmlNode
from Library.Xml.XmlRoutines import CreateXmlElement
from Library.Xml.XmlRoutines import XmlList
from Library.StringUtils import GetStringOfList
from Library.StringUtils import ConvertNEToNOTEQ
from Library.StringUtils import ConvertNOTEQToNE
from Library import GlobalData
from Object.POM.CommonObject import PcdObject
from Object.POM.CommonObject import PcdErrorObject
from Xml.CommonXml import HelpTextXml
from Xml.CommonXml import PromptXml
from Xml.CommonXml import CommonDefinesXml
from Xml.XmlParserMisc import GetHelpTextList
from Xml.XmlParserMisc import GetPromptList
import re

##
# PcdErrorXml
#
class PcdErrorXml(object):
    def __init__(self):
        self.ValidValueList = ''
        self.ValidValueListLang = ''
        self.ValidValueRange = ''
        self.Expression = ''
        self.ErrorNumber = ''
        self.ErrorMessage = []

    def FromXml(self, Item, Key):
        self.ValidValueList = XmlElement(Item, '%s/ValidValueList' % Key)
        self.ValidValueListLang = \
        XmlAttribute(XmlNode(Item, '%s/ValidValueList' % Key), 'Lang')
        self.ValidValueRange = self.TransferValidEpxr2ValidRange(XmlElement(Item, '%s/ValidValueRange' % Key))
        self.Expression = XmlElement(Item, '%s/Expression' % Key)
        self.ErrorNumber = XmlElement(Item, '%s/ErrorNumber' % Key)
        for ErrMsg in XmlList(Item, '%s/ErrorMessage' % Key):
            ErrorMessageString = XmlElement(ErrMsg, 'ErrorMessage')
            ErrorMessageLang = \
            XmlAttribute(XmlNode(ErrMsg, 'ErrorMessage'), 'Lang')
            self.ErrorMessage.append((ErrorMessageLang, ErrorMessageString))

        Error = PcdErrorObject()
        Error.SetValidValue(self.ValidValueList)
        Error.SetValidValueLang(self.ValidValueListLang)
        Error.SetValidValueRange(self.ValidValueRange)
        Error.SetExpression(self.Expression)
        Error.SetErrorNumber(self.ErrorNumber)
        Error.SetErrorMessageList(self.ErrorMessage)

        return Error

    def ToXml(self, PcdError, Key):
        if self.Expression:
            pass
        AttributeList = []
        NodeList = []
        if PcdError.GetValidValue():
            Element1 = \
            CreateXmlElement('ValidValueList', PcdError.GetValidValue(), [], \
                             [['Lang', PcdError.GetValidValueLang()]])
            NodeList.append(Element1)
        if PcdError.GetValidValueRange():
            TansferedRangeStr = self.TransferValidRange2Expr(PcdError.GetTokenSpaceGuidCName(),
                                                             PcdError.GetCName(),
                                                             PcdError.GetValidValueRange())
            Element1 = \
            CreateXmlElement('ValidValueRange', \
                             TansferedRangeStr, [], [])
            NodeList.append(Element1)
        if PcdError.GetExpression():
            NodeList.append(['Expression', PcdError.GetExpression()])
        if PcdError.GetErrorNumber():
            NodeList.append(['ErrorNumber', PcdError.GetErrorNumber()])
        for Item in PcdError.GetErrorMessageList():
            Element = \
            CreateXmlElement('ErrorMessage', Item[1], [], [['Lang', Item[0]]])
            NodeList.append(Element)
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def TransferValidRange2Expr(self, TokenSpaceGuidCName, CName, ValidRange):
        if self.Expression:
            pass
        INT_RANGE_PATTERN1 = '[\t\s]*[0-9]+[\t\s]*-[\t\s]*[0-9]+'
        INT_RANGE_PATTERN2 = '[\t\s]*(LT|GT|LE|GE|XOR|EQ)[\t\s]+\d+[\t\s]*'
        HEX_RANGE_PATTERN1 = \
            '[\t\s]*0[xX][a-fA-F0-9]+[\t\s]*-[\t\s]*0[xX][a-fA-F0-9]+'
        HEX_RANGE_PATTERN2 = '[\t\s]*(LT|GT|LE|GE|XOR|EQ)[\t\s]+0[xX][a-fA-F0-9]+[\t\s]*'
        IntMatch1 = re.compile(INT_RANGE_PATTERN1)
        IntMatch2 = re.compile(INT_RANGE_PATTERN2)
        HexMatch1 = re.compile(HEX_RANGE_PATTERN1)
        HexMatch2 = re.compile(HEX_RANGE_PATTERN2)
        PcdName = '.'.join([TokenSpaceGuidCName, CName])
        HexMatchedList = []
        IntMatchedList = []
        #
        # Convert HEX2 format range
        #
        if HexMatch2:
            for MatchObj in HexMatch2.finditer(ValidRange):
                MatchStr = MatchObj.group()
                TransferedRangeStr = ' '.join(['', PcdName, MatchStr.strip()])
                ValidRange = ValidRange.replace(MatchStr, TransferedRangeStr)
        #
        # Convert INT2 format range
        #
        if IntMatch2:
            for MatchObj in IntMatch2.finditer(ValidRange):
                MatchStr = MatchObj.group()
                TransferedRangeStr = ' '.join(['', PcdName, MatchStr.strip()])
                ValidRange = ValidRange.replace(MatchStr, TransferedRangeStr)
        #
        # Convert HEX1 format range
        #
        if HexMatch1:
            HexMatchedList += HexMatch1.findall(ValidRange)

        for MatchStr in HexMatchedList:
            RangeItemList = MatchStr.strip().split('-')
            TransferedRangeStr = '(%s GE %s) AND (%s LE %s)' % \
                (PcdName, RangeItemList[0].strip(), PcdName, RangeItemList[1].strip())
            ValidRange = ValidRange.replace(MatchStr, TransferedRangeStr)
        #
        # Convert INT1 format range
        #
        if IntMatch1:
            IntMatchedList += IntMatch1.findall(ValidRange)

        for MatchStr in IntMatchedList:
            RangeItemList = MatchStr.strip().split('-')
            TransferedRangeStr = '(%s GE %s) AND (%s LE %s)' % \
                (PcdName, RangeItemList[0].strip(), PcdName, RangeItemList[1].strip())
            ValidRange = ValidRange.replace(MatchStr, TransferedRangeStr)

        return ValidRange

    def TransferValidEpxr2ValidRange(self, ValidRangeExpr):
        if self.Expression:
            pass

        PCD_PATTERN = \
        '[\t\s]*[_a-zA-Z][a-zA-Z0-9_]*[\t\s]*\.[\t\s]*[_a-zA-Z][a-zA-Z0-9_]*[\t\s]*'
        IntPattern1 = \
        '[\t\s]*\([\t\s]*'+PCD_PATTERN+'[\t\s]+GE[\t\s]+\d+[\t\s]*\)[\t\s]+AND[\t\s]+\([\t\s]*'+\
        PCD_PATTERN+'[\t\s]+LE[\t\s]+\d+[\t\s]*\)'
        IntPattern1 = IntPattern1.replace(' ', '')
        IntPattern2 = '[\t\s]*'+PCD_PATTERN+'[\t\s]+(LT|GT|LE|GE|XOR|EQ)[\t\s]+\d+[\t\s]*'

        HexPattern1 = \
        '[\t\s]*\([\t\s]*'+PCD_PATTERN+'[\t\s]+GE[\t\s]+0[xX][0-9a-fA-F]+[\t\s]*\)[\t\s]+AND[\t\s]+\([\t\s]*'+\
        PCD_PATTERN+'[\t\s]+LE[\t\s]+0[xX][0-9a-fA-F]+[\t\s]*\)'
        HexPattern1 = HexPattern1.replace(' ', '')
        HexPattern2 = '[\t\s]*'+PCD_PATTERN+'[\t\s]+(LT|GT|LE|GE|XOR|EQ)[\t\s]+0[xX][0-9a-zA-Z]+[\t\s]*'

        #
        # Do the Hex1 conversion
        #
        HexMatchedList = re.compile(HexPattern1).findall(ValidRangeExpr)
        HexRangeDict = {}
        for HexMatchedItem in HexMatchedList:
            #
            # To match items on both sides of '-'
            #
            RangeItemList = re.compile('[\t\s]*0[xX][0-9a-fA-F]+[\t\s]*').findall(HexMatchedItem)
            if RangeItemList and len(RangeItemList) == 2:
                HexRangeDict[HexMatchedItem] = RangeItemList

        for Key in HexRangeDict.keys():
            MaxItem = MixItem = ''
            if int(HexRangeDict[Key][0], 16) > int(HexRangeDict[Key][1], 16):
                MaxItem = HexRangeDict[Key][0]
                MixItem = HexRangeDict[Key][1]
            else:
                MaxItem = HexRangeDict[Key][1]
                MixItem = HexRangeDict[Key][0]

            Range = ' %s - %s' % (MixItem.strip(), MaxItem.strip())
            ValidRangeExpr = ValidRangeExpr.replace(Key, Range)
        #
        # Do the INT1 conversion
        #
        IntRangeDict = {}
        IntMatchList = re.compile(IntPattern1).findall(ValidRangeExpr)
        for MatchedItem in IntMatchList:
            #
            # To match items on both sides of '-'
            #
            RangeItemList = re.compile('[\t\s]*\d+[\t\s]*').findall(MatchedItem)
            if RangeItemList and len(RangeItemList) == 2:
                IntRangeDict[MatchedItem] = RangeItemList

        for Key in IntRangeDict.keys():
            MaxItem = MixItem = ''
            if int(IntRangeDict[Key][0]) > int(IntRangeDict[Key][1]):
                MaxItem = IntRangeDict[Key][0]
                MixItem = IntRangeDict[Key][1]
            else:
                MaxItem = IntRangeDict[Key][1]
                MixItem = IntRangeDict[Key][0]

            Range = ' %s - %s' % (MixItem.strip(), MaxItem.strip())
            ValidRangeExpr = ValidRangeExpr.replace(Key, Range)
        #
        # Do the HEX2 conversion
        #
        for MatchObj in re.compile(HexPattern2).finditer(ValidRangeExpr):
            MatchStr = MatchObj.group()
            Range = re.compile(PCD_PATTERN).sub(' ', MatchStr)
            ValidRangeExpr = ValidRangeExpr.replace(MatchStr, Range)
        #
        # Do the INT2 conversion
        #
        for MatchObj in re.compile(IntPattern2).finditer(ValidRangeExpr):
            MatchStr = MatchObj.group()
            Range = re.compile(PCD_PATTERN).sub(' ', MatchStr)
            ValidRangeExpr = ValidRangeExpr.replace(MatchStr, Range)

        return ValidRangeExpr



    def __str__(self):
        return "ValidValueList = %s ValidValueListLang = %s ValidValueRange \
        = %s Expression = %s ErrorNumber = %s %s" % \
        (self.ValidValueList, self.ValidValueListLang, self.ValidValueRange, \
         self.Expression, self.ErrorNumber, self.ErrorMessage)

##
# PcdEntryXml
#
class PcdEntryXml(object):
    def __init__(self):
        self.PcdItemType = ''
        self.PcdUsage = ''
        self.TokenSpaceGuidCName = ''
        self.TokenSpaceGuidValue = ''
        self.Token = ''
        self.CName = ''
        self.PcdCName = ''
        self.DatumType = ''
        self.ValidUsage = ''
        self.DefaultValue = ''
        self.MaxDatumSize = ''
        self.Value = ''
        self.Offset = ''
        self.CommonDefines = CommonDefinesXml()
        self.Prompt = []
        self.HelpText = []
        self.PcdError = []

    ##
    # AsBuilt will use FromXml
    #
    def FromXml(self, Item, Key):
        self.PcdItemType = \
        XmlAttribute(XmlNode(Item, '%s' % Key), 'PcdItemType')
        self.PcdUsage = XmlAttribute(XmlNode(Item, '%s' % Key), 'PcdUsage')
        self.TokenSpaceGuidCName = \
        XmlElement(Item, '%s/TokenSpaceGuidCname' % Key)
        self.TokenSpaceGuidValue = \
        XmlElement(Item, '%s/TokenSpaceGuidValue' % Key)
        self.Token = XmlElement(Item, '%s/Token' % Key)
        self.CName = XmlElement(Item, '%s/CName' % Key)
        self.PcdCName = XmlElement(Item, '%s/PcdCName' % Key)
        self.DatumType = XmlElement(Item, '%s/DatumType' % Key)
        self.ValidUsage = XmlElement(Item, '%s/ValidUsage' % Key)
        if not GlobalData.gIS_BINARY_INF:
            self.DefaultValue = XmlElement(Item, '%s/DefaultValue' % Key)
        else:
            self.DefaultValue = XmlElement(Item, '%s/Value' % Key)
        self.MaxDatumSize = XmlElement(Item, '%s/MaxDatumSize' % Key)
        self.Value = XmlElement(Item, '%s/Value' % Key)
        self.Offset = XmlElement(Item, '%s/Offset' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)

        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        for PcdErrorItem in XmlList(Item, '%s/PcdError' % Key):
            PcdErrorObjXml = PcdErrorXml()
            PcdErrorObj = PcdErrorObjXml.FromXml(PcdErrorItem, 'PcdError')
            self.PcdError.append(PcdErrorObj)

        self.DefaultValue = ConvertNOTEQToNE(self.DefaultValue)

        PcdEntry = PcdObject()
        PcdEntry.SetSupArchList(self.CommonDefines.SupArchList)
        PcdEntry.SetTokenSpaceGuidCName(self.TokenSpaceGuidCName)
        PcdEntry.SetTokenSpaceGuidValue(self.TokenSpaceGuidValue)
        PcdEntry.SetToken(self.Token)
        PcdEntry.SetOffset(self.Offset)
        PcdEntry.SetCName(self.CName)
        PcdEntry.SetPcdCName(self.PcdCName)
        PcdEntry.SetDatumType(self.DatumType)
        PcdEntry.SetValidUsage(self.ValidUsage)
        PcdEntry.SetDefaultValue(self.DefaultValue)
        PcdEntry.SetMaxDatumSize(self.MaxDatumSize)
        PcdEntry.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))
        PcdEntry.SetItemType(self.PcdItemType)

        PcdEntry.SetHelpTextList(GetHelpTextList(self.HelpText))
        PcdEntry.SetPcdErrorsList(self.PcdError)

        return PcdEntry
    ##
    # Package will use FromXml2
    #
    def FromXml2(self, Item, Key):
        self.TokenSpaceGuidCName = \
        XmlElement(Item, '%s/TokenSpaceGuidCname' % Key)
        self.Token = XmlElement(Item, '%s/Token' % Key)
        self.CName = XmlElement(Item, '%s/CName' % Key)
        self.DatumType = XmlElement(Item, '%s/DatumType' % Key)
        self.ValidUsage = XmlElement(Item, '%s/ValidUsage' % Key)
        self.DefaultValue = XmlElement(Item, '%s/DefaultValue' % Key)
        self.MaxDatumSize = XmlElement(Item, '%s/MaxDatumSize' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)
        for PromptItem in XmlList(Item, '%s/Prompt' % Key):
            PromptObj = PromptXml()
            PromptObj.FromXml(PromptItem, '%s/Prompt' % Key)
            self.Prompt.append(PromptObj)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        for PcdErrorItem in XmlList(Item, '%s/PcdError' % Key):
            PcdErrorObjXml = PcdErrorXml()
            PcdErrorObj = PcdErrorObjXml.FromXml(PcdErrorItem, 'PcdError')
            self.PcdError.append(PcdErrorObj)

        self.DefaultValue = ConvertNOTEQToNE(self.DefaultValue)

        PcdEntry = PcdObject()
        PcdEntry.SetSupArchList(self.CommonDefines.SupArchList)
        PcdEntry.SetSupModuleList(self.CommonDefines.SupModList)
        PcdEntry.SetTokenSpaceGuidCName(self.TokenSpaceGuidCName)
        PcdEntry.SetToken(self.Token)
        PcdEntry.SetCName(self.CName)
        PcdEntry.SetDatumType(self.DatumType)
        PcdEntry.SetValidUsage(self.ValidUsage)
        PcdEntry.SetDefaultValue(self.DefaultValue)
        PcdEntry.SetMaxDatumSize(self.MaxDatumSize)
        PcdEntry.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))

        PcdEntry.SetPromptList(GetPromptList(self.Prompt))
        PcdEntry.SetHelpTextList(GetHelpTextList(self.HelpText))
        PcdEntry.SetPcdErrorsList(self.PcdError)

        return PcdEntry

    ##
    # Module will use FromXml3
    #
    def FromXml3(self, Item, Key):
        self.PcdItemType = \
        XmlAttribute(XmlNode(Item, '%s' % Key), 'PcdItemType')
        self.PcdUsage = XmlAttribute(XmlNode(Item, '%s' % Key), 'PcdUsage')
        self.TokenSpaceGuidCName = \
        XmlElement(Item, '%s/TokenSpaceGuidCName' % Key)
        self.CName = XmlElement(Item, '%s/CName' % Key)
        self.DefaultValue = XmlElement(Item, '%s/DefaultValue' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        for PcdErrorItem in XmlList(Item, '%s/PcdError' % Key):
            PcdErrorObj = PcdErrorXml()
            PcdErrorObj.FromXml(PcdErrorItem, 'PcdError')
            self.PcdError.append(PcdErrorObj)

        self.DefaultValue = ConvertNOTEQToNE(self.DefaultValue)

        PcdEntry = PcdObject()
        PcdEntry.SetSupArchList(self.CommonDefines.SupArchList)
        PcdEntry.SetTokenSpaceGuidCName(self.TokenSpaceGuidCName)
        PcdEntry.SetCName(self.CName)
        PcdEntry.SetValidUsage(self.PcdUsage)
        PcdEntry.SetDefaultValue(self.DefaultValue)
        PcdEntry.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))
        PcdEntry.SetItemType(self.PcdItemType)

        PcdEntry.SetHelpTextList(GetHelpTextList(self.HelpText))
        PcdEntry.SetPcdErrorsList(self.PcdError)

        return PcdEntry

    def ToXml(self, PcdEntry, Key):
        if self.PcdCName:
            pass

        DefaultValue = ConvertNEToNOTEQ(PcdEntry.GetDefaultValue())

        AttributeList = \
        [['SupArchList', GetStringOfList(PcdEntry.GetSupArchList())], \
         ['PcdUsage', PcdEntry.GetValidUsage()], \
         ['PcdItemType', PcdEntry.GetItemType()], \
         ['FeatureFlag', PcdEntry.GetFeatureFlag()],
        ]
        NodeList = [['TokenSpaceGuidCname', PcdEntry.GetTokenSpaceGuidCName()],
                    ['TokenSpaceGuidValue', PcdEntry.GetTokenSpaceGuidValue()],
                    ['Token', PcdEntry.GetToken()],
                    ['CName', PcdEntry.GetCName()],
                    ['DatumType', PcdEntry.GetDatumType()],
                    ['ValidUsage', GetStringOfList(PcdEntry.GetValidUsage())],
                    ['DefaultValue', DefaultValue],
                    ['MaxDatumSize', PcdEntry.GetMaxDatumSize()],
                    ['Offset', PcdEntry.GetOffset()],
                   ]

        for Item in PcdEntry.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        for Item in PcdEntry.GetPcdErrorsList():
            Tmp = PcdErrorXml()
            NodeList.append(Tmp.ToXml(Item, 'PcdError'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root
    ##
    # Package will use ToXml2
    #
    def ToXml2(self, PcdEntry, Key):
        if self.PcdCName:
            pass

        DefaultValue = ConvertNEToNOTEQ(PcdEntry.GetDefaultValue())

        AttributeList = \
        [['SupArchList', GetStringOfList(PcdEntry.GetSupArchList())], \
         ['SupModList', GetStringOfList(PcdEntry.GetSupModuleList())]
        ]
        NodeList = [['TokenSpaceGuidCname', PcdEntry.GetTokenSpaceGuidCName()],
                    ['Token', PcdEntry.GetToken()],
                    ['CName', PcdEntry.GetCName()],
                    ['DatumType', PcdEntry.GetDatumType()],
                    ['ValidUsage', GetStringOfList(PcdEntry.GetValidUsage())],
                    ['DefaultValue', DefaultValue],
                    ['MaxDatumSize', PcdEntry.GetMaxDatumSize()],
                   ]
        for Item in PcdEntry.GetPromptList():
            Tmp = PromptXml()
            NodeList.append(Tmp.ToXml(Item))

        for Item in PcdEntry.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))

        for Item in PcdEntry.GetPcdErrorsList():
            Tmp = PcdErrorXml()
            NodeList.append(Tmp.ToXml(Item, 'PcdError'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root
    ##
    # Module will use ToXml3
    #
    def ToXml3(self, PcdEntry, Key):
        if self.PcdCName:
            pass

        DefaultValue = ConvertNEToNOTEQ(PcdEntry.GetDefaultValue())

        AttributeList = \
        [['SupArchList', GetStringOfList(PcdEntry.GetSupArchList())], \
         ['PcdUsage', PcdEntry.GetValidUsage()], \
         ['PcdItemType', PcdEntry.GetItemType()], \
         ['FeatureFlag', ConvertNEToNOTEQ(PcdEntry.GetFeatureFlag())],
        ]
        NodeList = [['CName', PcdEntry.GetCName()],
                    ['TokenSpaceGuidCName', PcdEntry.GetTokenSpaceGuidCName()],
                    ['DefaultValue', DefaultValue],
                   ]

        for Item in PcdEntry.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        for Item in PcdEntry.GetPcdErrorsList():
            Tmp = PcdErrorXml()
            NodeList.append(Tmp.ToXml(Item, 'PcdError'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    ##
    # AsBuild Module will use ToXml4
    #
    def ToXml4(self, PcdEntry, Key):
        if self.PcdCName:
            pass

        DefaultValue = ConvertNEToNOTEQ(PcdEntry.GetDefaultValue())

        AttributeList = []

        NodeList = [
                    ['TokenSpaceGuidValue', PcdEntry.GetTokenSpaceGuidValue()],
                    ['PcdCName', PcdEntry.GetCName()],
                    ['Token', PcdEntry.GetToken()],
                    ['DatumType', PcdEntry.GetDatumType()],
                    ['MaxDatumSize', PcdEntry.GetMaxDatumSize()],
                    ['Value', DefaultValue],
                    ['Offset', PcdEntry.GetOffset()]
                   ]

        for Item in PcdEntry.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        for Item in PcdEntry.GetPcdErrorsList():
            Tmp = PcdErrorXml()
            NodeList.append(Tmp.ToXml(Item, 'PcdError'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root


    def __str__(self):
        Str = \
        ('PcdItemType = %s PcdUsage = %s TokenSpaceGuidCName = %s \
        TokenSpaceGuidValue = %s Token = %s CName = %s PcdCName = %s \
        DatumType = %s ValidUsage = %s DefaultValue = %s MaxDatumSize = %s \
        Value = %s Offset = %s %s') % \
        (self.PcdItemType, self.PcdUsage, self.TokenSpaceGuidCName, \
         self.TokenSpaceGuidValue, self.Token, self.CName, self.PcdCName, \
         self.DatumType, self.ValidUsage, self.DefaultValue, \
         self.MaxDatumSize, self.Value, self.Offset, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        for Item in self.PcdError:
            Str = Str + "\n\tPcdError:" + str(Item)
        return Str
