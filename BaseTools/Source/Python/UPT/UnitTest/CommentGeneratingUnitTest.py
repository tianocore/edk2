## @file
# This file contain unit test for CommentParsing
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

import os
import unittest

import Logger.Log as Logger
from GenMetaFile.GenInfFile import GenGuidSections
from GenMetaFile.GenInfFile import GenProtocolPPiSections
from GenMetaFile.GenInfFile import GenPcdSections
from GenMetaFile.GenInfFile import GenSpecialSections
from Library.CommentGenerating import GenGenericCommentF
from Library.CommentGenerating import _GetHelpStr
from Object.POM.CommonObject import TextObject
from Object.POM.CommonObject import GuidObject
from Object.POM.CommonObject import ProtocolObject
from Object.POM.CommonObject import PpiObject
from Object.POM.CommonObject import PcdObject
from Object.POM.ModuleObject import HobObject

from Library.StringUtils import GetSplitValueList
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_LANGUAGE_EN_US
from Library.DataType import TAB_LANGUAGE_ENG
from Library.DataType import ITEM_UNDEFINED
from Library.DataType import TAB_INF_FEATURE_PCD
from Library import GlobalData
from Library.Misc import CreateDirectory

#
# Test _GetHelpStr
#
class _GetHelpStrTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # Normal case1: have one help text object with Lang = 'en-US'
    #
    def testNormalCase1(self):
        HelpStr = 'Hello world'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(TAB_LANGUAGE_EN_US)
        HelpTextObj.SetString(HelpStr)

        HelpTextList = [HelpTextObj]
        Result = _GetHelpStr(HelpTextList)
        self.assertEqual(Result, HelpStr)

    #
    # Normal case2: have two help text object with Lang = 'en-US' and other
    #
    def testNormalCase2(self):
        HelpStr = 'Hello world'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(TAB_LANGUAGE_ENG)
        HelpTextObj.SetString(HelpStr)

        HelpTextList = [HelpTextObj]

        ExpectedStr = 'Hello world1'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(TAB_LANGUAGE_EN_US)
        HelpTextObj.SetString(ExpectedStr)

        HelpTextList.append(HelpTextObj)

        Result = _GetHelpStr(HelpTextList)
        self.assertEqual(Result, ExpectedStr)

    #
    # Normal case3: have two help text object with Lang = '' and 'eng'
    #
    def testNormalCase3(self):
        HelpStr = 'Hello world'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)

        HelpTextList = [HelpTextObj]

        ExpectedStr = 'Hello world1'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(TAB_LANGUAGE_ENG)
        HelpTextObj.SetString(ExpectedStr)

        HelpTextList.append(HelpTextObj)

        Result = _GetHelpStr(HelpTextList)
        self.assertEqual(Result, ExpectedStr)

    #
    # Normal case4: have two help text object with Lang = '' and ''
    #
    def testNormalCase4(self):

        ExpectedStr = 'Hello world1'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(TAB_LANGUAGE_ENG)
        HelpTextObj.SetString(ExpectedStr)
        HelpTextList = [HelpTextObj]

        HelpStr = 'Hello world'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        HelpTextList.append(HelpTextObj)

        Result = _GetHelpStr(HelpTextList)
        self.assertEqual(Result, ExpectedStr)

    #
    # Normal case: have three help text object with Lang = '','en', 'en-US'
    #
    def testNormalCase5(self):

        ExpectedStr = 'Hello world1'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(TAB_LANGUAGE_EN_US)
        HelpTextObj.SetString(ExpectedStr)
        HelpTextList = [HelpTextObj]

        HelpStr = 'Hello unknown world'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        HelpTextList.append(HelpTextObj)

        HelpStr = 'Hello mysterious world'
        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        HelpTextList.append(HelpTextObj)

        Result = _GetHelpStr(HelpTextList)
        self.assertEqual(Result, ExpectedStr)

        HelpTextList.sort()
        self.assertEqual(Result, ExpectedStr)

        HelpTextList.sort(reverse=True)
        self.assertEqual(Result, ExpectedStr)


#
# Test GenGuidSections
#
class GenGuidSectionsTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # This is the API to generate Guid Object to help UnitTest
    #
    def GuidFactory(self, CName, FFE, Usage, GuidType, VariableName, HelpStr):
        Guid = GuidObject()
        Guid.SetCName(CName)
        Guid.SetFeatureFlag(FFE)
        Guid.SetGuidTypeList([GuidType])
        Guid.SetUsage(Usage)
        Guid.SetVariableName(VariableName)

        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        Guid.SetHelpTextList([HelpTextObj])

        return Guid

    #
    # Normal case: have two GuidObject
    #
    def testNormalCase1(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 1'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'CONSUMES'
        GuidType = 'Variable'
        VariableName = ''
        HelpStr = 'Usage comment line 2'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
## PRODUCES ## Event # Usage comment line 1
## CONSUMES ## Variable: # Usage comment line 2
Guid1|FFE1'''
        self.assertEqual(Result.strip(), Expected)

    #
    # Normal case: have two GuidObject
    #
    def testNormalCase2(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 1'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = 'Generic comment line 1\n Generic comment line 2'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
## PRODUCES ## Event # Usage comment line 1
# Generic comment line 1
# Generic comment line 2
Guid1|FFE1'''

        self.assertEqual(Result.strip(), Expected)

    #
    # Normal case: have two GuidObject, one help goes to generic help,
    # the other go into usage comment
    #
    def testNormalCase3(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = 'Generic comment'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 1'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
# Generic comment
## PRODUCES ## Event # Usage comment line 1
Guid1|FFE1'''

        self.assertEqual(Result.strip(), Expected)

    #
    # Normal case: have one GuidObject, generic comment multiple lines
    #
    def testNormalCase5(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = 'Generic comment line1 \n generic comment line 2'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
# Generic comment line1
# generic comment line 2
Guid1|FFE1'''

        self.assertEqual(Result.strip(), Expected)

    #
    # Normal case: have one GuidObject, usage comment multiple lines
    #
    def testNormalCase6(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 1\n Usage comment line 2'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
Guid1|FFE1 ## PRODUCES ## Event # Usage comment line 1  Usage comment line 2
'''
        self.assertEqual(Result.strip(), Expected.strip())

    #
    # Normal case: have one GuidObject, usage comment one line
    #
    def testNormalCase7(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = 'Usage comment line 1'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
Guid1|FFE1 # Usage comment line 1
'''
        self.assertEqual(Result.strip(), Expected.strip())

    #
    # Normal case: have two GuidObject
    #
    def testNormalCase8(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 1\n Usage comment line 2'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 3'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
## PRODUCES ## Event # Usage comment line 1  Usage comment line 2
## PRODUCES ## Event # Usage comment line 3
Guid1|FFE1
'''
        self.assertEqual(Result.strip(), Expected.strip())

    #
    # Normal case: have no GuidObject
    #
    def testNormalCase9(self):
        GuidList = []

        Result = GenGuidSections(GuidList)
        Expected = ''
        self.assertEqual(Result.strip(), Expected.strip())

    #
    # Normal case: have one GuidObject with no comment generated
    #
    def testNormalCase10(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = ''
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
Guid1|FFE1
'''
        self.assertEqual(Result.strip(), Expected.strip())

    #
    # Normal case: have three GuidObject
    #
    def testNormalCase11(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = 'general comment line 1'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = 'Usage comment line 3'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'UNDEFINED'
        GuidType = 'UNDEFINED'
        VariableName = ''
        HelpStr = 'general comment line 2'
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
# general comment line 1
## PRODUCES ## Event # Usage comment line 3
# general comment line 2
Guid1|FFE1
'''
        self.assertEqual(Result.strip(), Expected.strip())

    #
    # Normal case: have three GuidObject, with Usage/Type and no help
    #
    def testNormalCase12(self):
        GuidList = []

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'GUID'
        VariableName = ''
        HelpStr = ''
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'PRODUCES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = ''
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        CName = 'Guid1'
        FFE = 'FFE1'
        Usage = 'CONSUMES'
        GuidType = 'Event'
        VariableName = ''
        HelpStr = ''
        Guid1 = self.GuidFactory(CName, FFE, Usage, GuidType,
                                 VariableName, HelpStr)
        GuidList.append(Guid1)

        Result = GenGuidSections(GuidList)
        Expected = '''[Guids]
## PRODUCES ## GUID
## PRODUCES ## Event
## CONSUMES ## Event
Guid1|FFE1
'''
        self.assertEqual(Result.strip(), Expected.strip())

#
# Test GenProtocolPPiSections
#
class GenProtocolPPiSectionsTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # This is the API to generate Protocol/Ppi Object to help UnitTest
    #
    def ObjectFactory(self, CName, FFE, Usage, Notify, HelpStr, IsProtocol):
        if IsProtocol:
            Object = ProtocolObject()
        else:
            Object = PpiObject()

        Object.SetCName(CName)
        Object.SetFeatureFlag(FFE)
        Object.SetUsage(Usage)
        Object.SetNotify(Notify)

        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        Object.SetHelpTextList([HelpTextObj])

        return Object

    #    Usage        Notify    Help    INF Comment
    #1   UNDEFINED    true    Present    ## UNDEFINED ## NOTIFY # Help
    #2   UNDEFINED    true    Not Present    ## UNDEFINED ## NOTIFY
    #3   UNDEFINED    false    Present    ## UNDEFINED # Help
    #4   UNDEFINED    false     Not Present    ## UNDEFINED
    #5   UNDEFINED    Not Present    Present    # Help
    #6   UNDEFINED    Not Present    Not Present    <empty>
    #7   Other        true    Present    ## Other ## NOTIFY # Help
    #8   Other        true    Not Present    ## Other ## NOTIFY
    #9   Other        false    Present    ## Other # Help
    #A   Other        false     Not Present    ## Other
    #B   Other        Not Present    Present    ## Other # Help
    #C   Other        Not Present    Not Present    ## Other

    def testNormalCase1(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'UNDEFINED'
        Notify = True
        HelpStr = 'Help'
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## UNDEFINED ## NOTIFY # Help'''
        self.assertEqual(Result.strip(), Expected)

        IsProtocol = False
        ObjectList = []
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Ppis]
Guid1|FFE1 ## UNDEFINED ## NOTIFY # Help'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase2(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'UNDEFINED'
        Notify = True
        HelpStr = ''
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## UNDEFINED ## NOTIFY'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase3(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'UNDEFINED'
        Notify = False
        HelpStr = 'Help'
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## UNDEFINED # Help'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase4(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'UNDEFINED'
        Notify = False
        HelpStr = ''
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## UNDEFINED'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase5(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'UNDEFINED'
        Notify = ''
        HelpStr = 'Help'
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 # Help'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase6(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'UNDEFINED'
        Notify = ''
        HelpStr = ''
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase7(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'PRODUCES'
        Notify = True
        HelpStr = 'Help'
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## PRODUCES ## NOTIFY # Help'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase8(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'PRODUCES'
        Notify = True
        HelpStr = ''
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## PRODUCES ## NOTIFY'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase9(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'PRODUCES'
        Notify = False
        HelpStr = 'Help'
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## PRODUCES # Help'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCaseA(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'PRODUCES'
        Notify = False
        HelpStr = ''
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## PRODUCES'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCaseB(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'PRODUCES'
        Notify = ''
        HelpStr = 'Help'
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## PRODUCES # Help'''
        self.assertEqual(Result.strip(), Expected)

    def testNormalCaseC(self):
        ObjectList = []

        CName = 'Guid1'
        FFE = 'FFE1'

        Usage = 'PRODUCES'
        Notify = ''
        HelpStr = ''
        IsProtocol = True
        Object = self.ObjectFactory(CName, FFE, Usage, Notify,
                                 HelpStr, IsProtocol)
        ObjectList.append(Object)


        Result = GenProtocolPPiSections(ObjectList, IsProtocol)
        Expected = '''[Protocols]
Guid1|FFE1 ## PRODUCES'''
        self.assertEqual(Result.strip(), Expected)

#
# Test GenPcdSections
#
class GenPcdSectionsTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # This is the API to generate Pcd Object to help UnitTest
    #
    def ObjectFactory(self, ItemType, TSCName, CName, DValue, FFE, Usage, Str):
        Object = PcdObject()
        HelpStr = Str

        Object.SetItemType(ItemType)
        Object.SetTokenSpaceGuidCName(TSCName)
        Object.SetCName(CName)
        Object.SetDefaultValue(DValue)
        Object.SetFeatureFlag(FFE)
        Object.SetValidUsage(Usage)

        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        Object.SetHelpTextList([HelpTextObj])

        return Object


    #    Usage        Help    INF Comment
    #1   UNDEFINED    Present    # Help
    #2   UNDEFINED    Not Present    <empty>
    #3   Other        Present    ## Other # Help
    #4   Other        Not Present    ## Other

    def testNormalCase1(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'UNDEFINED'
        Str = 'Help'

        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = \
            '[Pcd]\n' + \
            'TSCName.CName|DValue|FFE # Help'
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase2(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'UNDEFINED'
        Str = ''

        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '[Pcd]\nTSCName.CName|DValue|FFE'
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase3(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'CONSUMES'
        Str = 'Help'

        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '[Pcd]\nTSCName.CName|DValue|FFE ## CONSUMES # Help'
        self.assertEqual(Result.strip(), Expected)

    def testNormalCase4(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'CONSUMES'
        Str = ''

        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '[Pcd]\nTSCName.CName|DValue|FFE ## CONSUMES'
        self.assertEqual(Result.strip(), Expected)

    #
    # multiple lines for normal usage
    #
    def testNormalCase5(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'CONSUMES'
        Str = 'commment line 1\ncomment line 2'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '''[Pcd]
TSCName.CName|DValue|FFE ## CONSUMES # commment line 1 comment line 2'''
        self.assertEqual(Result.strip(), Expected)

    #
    # multiple lines for UNDEFINED usage
    #
    def testNormalCase6(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'UNDEFINED'
        Str = 'commment line 1\ncomment line 2'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Usage = 'UNDEFINED'
        Str = 'commment line 3'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '''[Pcd]
# commment line 1
# comment line 2
# commment line 3
TSCName.CName|DValue|FFE'''
        self.assertEqual(Result.strip(), Expected)

    #
    # multiple lines for UNDEFINED and normal usage
    #
    def testNormalCase7(self):
        ObjectList = []
        ItemType = 'Pcd'
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'UNDEFINED'
        Str = 'commment line 1\ncomment line 2'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Usage = 'CONSUMES'
        Str = 'Foo'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Usage = 'UNDEFINED'
        Str = 'commment line 3'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '''[Pcd]
# commment line 1
# comment line 2
## CONSUMES # Foo
# commment line 3
TSCName.CName|DValue|FFE'''
        self.assertEqual(Result.strip(), Expected)

    #    Usage        Help    INF Comment
    #    CONSUMES    Present    # Help (keep <EOL> and insert '#' at beginning of each new line)
    #    CONSUMES    Not Present   <empty>

    #
    # TAB_INF_FEATURE_PCD
    #
    def testNormalCase8(self):
        ObjectList = []
        ItemType = TAB_INF_FEATURE_PCD
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'CONSUMES'
        Str = 'commment line 1\ncomment line 2'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '''[FeaturePcd]
# commment line 1
# comment line 2
TSCName.CName|DValue|FFE'''
        self.assertEqual(Result.strip(), Expected)

    #
    # TAB_INF_FEATURE_PCD
    #
    def testNormalCase9(self):
        ObjectList = []
        ItemType = TAB_INF_FEATURE_PCD
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'CONSUMES'
        Str = ''
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '''[FeaturePcd]
TSCName.CName|DValue|FFE'''
        self.assertEqual(Result.strip(), Expected)

    #
    # TAB_INF_FEATURE_PCD
    #
    def testNormalCase10(self):
        ObjectList = []
        ItemType = TAB_INF_FEATURE_PCD
        TSCName = 'TSCName'
        CName = 'CName'
        DValue = 'DValue'
        FFE = 'FFE'

        Usage = 'PRODUCES'
        Str = 'commment line 1\ncomment line 2'
        Object = self.ObjectFactory(ItemType, TSCName, CName, DValue, FFE,
                                    Usage, Str)
        ObjectList.append(Object)

        Result = GenPcdSections(ObjectList)
        Expected = '''

[FeaturePcd]
# commment line 1
# comment line 2
TSCName.CName|DValue|FFE
'''
        self.assertEqual(Result, Expected)


#
# Test GenSpecialSections of Hob
#
class GenHobSectionsTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # This is the API to generate Event Object to help UnitTest
    #
    def ObjectFactory(self, SupArchList, Type, Usage, Str):
        Object = HobObject()
        HelpStr = Str

        Object.SetHobType(Type)
        Object.SetUsage(Usage)
        Object.SetSupArchList(SupArchList)

        HelpTextObj = TextObject()
        HelpTextObj.SetLang('')
        HelpTextObj.SetString(HelpStr)
        Object.SetHelpTextList([HelpTextObj])

        return Object

    def testNormalCase1(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = 'Help'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# # Help
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase2(self):
        ObjectList = []
        SupArchList = []
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = 'Help'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob]
# ##
# # Help
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase3(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\nComment Line 1\n\n'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# # Comment Line 1
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase4(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\nComment Line 1\n'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# # Comment Line 1
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase5(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = 'Comment Line 1\n\n'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# # Comment Line 1
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase6(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = ''

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase7(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\nNew Stack HoB'


        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# # New Stack HoB
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase8(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\nNew Stack HoB\n\nTail Comment'


        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# # New Stack HoB
# #
# # Tail Comment
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase9(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\n\n'


        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# #
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase10(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\n'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# #
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase11(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\n\n\n'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# #
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

    def testNormalCase12(self):
        ObjectList = []
        SupArchList = ['X64']
        Type = 'Foo'
        Usage = 'UNDEFINED'
        Str = '\n\n\n\n'

        Object = self.ObjectFactory(SupArchList, Type, Usage, Str)
        ObjectList.append(Object)

        Result = GenSpecialSections(ObjectList, 'Hob')
        Expected = '''# [Hob.X64]
# ##
# #
# #
# #
# Foo ## UNDEFINED
#
#
'''
        self.assertEqual(Result, Expected)

#
# Test GenGenericCommentF
#
class GenGenericCommentFTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testNormalCase1(self):
        CommentLines = 'Comment Line 1'
        Result = GenGenericCommentF(CommentLines)
        Expected = '# Comment Line 1\n'
        self.assertEqual(Result, Expected)

    def testNormalCase2(self):
        CommentLines = '\n'
        Result = GenGenericCommentF(CommentLines)
        Expected = '#\n'
        self.assertEqual(Result, Expected)

    def testNormalCase3(self):
        CommentLines = '\n\n\n'
        Result = GenGenericCommentF(CommentLines)
        Expected = '#\n#\n#\n'
        self.assertEqual(Result, Expected)

    def testNormalCase4(self):
        CommentLines = 'coment line 1\n'
        Result = GenGenericCommentF(CommentLines)
        Expected = '# coment line 1\n'
        self.assertEqual(Result, Expected)

    def testNormalCase5(self):
        CommentLines = 'coment line 1\n coment line 2\n'
        Result = GenGenericCommentF(CommentLines)
        Expected = '# coment line 1\n# coment line 2\n'
        self.assertEqual(Result, Expected)

if __name__ == '__main__':
    Logger.Initialize()
    unittest.main()
