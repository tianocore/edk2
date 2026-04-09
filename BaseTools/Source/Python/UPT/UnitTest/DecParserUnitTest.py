## @file
# This file contain unit test for DecParser
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

import os
import unittest
from Logger.Log import FatalError

from Parser.DecParser import \
    Dec, \
    _DecDefine, \
    _DecLibraryclass, \
    _DecPcd, \
    _DecGuid, \
    FileContent, \
    _DecBase, \
    CleanString

from Object.Parser.DecObject import _DecComments

#
# Test CleanString
#
class CleanStringTestCase(unittest.TestCase):
    def testCleanString(self):
        Line, Comment = CleanString('')
        self.assertEqual(Line, '')
        self.assertEqual(Comment, '')

        Line, Comment = CleanString('line without comment')
        self.assertEqual(Line, 'line without comment')
        self.assertEqual(Comment, '')

        Line, Comment = CleanString('# pure comment')
        self.assertEqual(Line, '')
        self.assertEqual(Comment, '# pure comment')

        Line, Comment = CleanString('line # and comment')
        self.assertEqual(Line, 'line')
        self.assertEqual(Comment, '# and comment')

    def testCleanStringCpp(self):
        Line, Comment = CleanString('line // and comment', AllowCppStyleComment = True)
        self.assertEqual(Line, 'line')
        self.assertEqual(Comment, '# and comment')

#
# Test _DecBase._MacroParser function
#
class MacroParserTestCase(unittest.TestCase):
    def setUp(self):
        self.dec = _DecBase(FileContent('dummy', []))

    def testCorrectMacro(self):
        self.dec._MacroParser('DEFINE MACRO1 = test1')
        self.failIf('MACRO1' not in self.dec._LocalMacro)
        self.assertEqual(self.dec._LocalMacro['MACRO1'], 'test1')

    def testErrorMacro1(self):
        # Raise fatal error, macro name must be upper case letter
        self.assertRaises(FatalError, self.dec._MacroParser, 'DEFINE not_upper_case = test2')

    def testErrorMacro2(self):
        # No macro name given
        self.assertRaises(FatalError, self.dec._MacroParser, 'DEFINE ')

#
# Test _DecBase._TryBackSlash function
#
class TryBackSlashTestCase(unittest.TestCase):
    def setUp(self):
        Content = [
            # Right case
            'test no backslash',

            'test with backslash \\',
            'continue second line',

            # Do not precede with whitespace
            '\\',

            # Empty line after backlash is not allowed
            'line with backslash \\',
            ''
        ]
        self.dec = _DecBase(FileContent('dummy', Content))

    def testBackSlash(self):
        #
        # Right case, assert return values
        #
        ConcatLine, CommentList = self.dec._TryBackSlash(self.dec._RawData.GetNextLine(), [])
        self.assertEqual(ConcatLine, 'test no backslash')
        self.assertEqual(CommentList, [])

        ConcatLine, CommentList = self.dec._TryBackSlash(self.dec._RawData.GetNextLine(), [])
        self.assertEqual(CommentList, [])
        self.assertEqual(ConcatLine, 'test with backslash continue second line')

        #
        # Error cases, assert raise exception
        #
        self.assertRaises(FatalError, self.dec._TryBackSlash, self.dec._RawData.GetNextLine(), [])
        self.assertRaises(FatalError, self.dec._TryBackSlash, self.dec._RawData.GetNextLine(), [])

#
# Test _DecBase.Parse function
#
class DataItem(_DecComments):
    def __init__(self):
        _DecComments.__init__(self)
        self.String = ''

class Data(_DecComments):
    def __init__(self):
        _DecComments.__init__(self)
        # List of DataItem
        self.ItemList = []

class TestInner(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.ItemObject = Data()

    def _StopCurrentParsing(self, Line):
        return Line == '[TOP]'

    def _ParseItem(self):
        Item = DataItem()
        Item.String = self._RawData.CurrentLine
        self.ItemObject.ItemList.append(Item)
        return Item

    def _TailCommentStrategy(self, Comment):
        return Comment.find('@comment') != -1

class TestTop(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        # List of Data
        self.ItemObject = []

    # Top parser
    def _StopCurrentParsing(self, Line):
        return False

    def _ParseItem(self):
        TestParser = TestInner(self._RawData)
        TestParser.Parse()
        self.ItemObject.append(TestParser.ItemObject)
        return TestParser.ItemObject

class ParseTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def testParse(self):
        Content = \
        '''# Top comment
        [TOP]
          # sub1 head comment
          (test item has both head and tail comment) # sub1 tail comment
          # sub2 head comment
          (test item has head and special tail comment)
          # @comment test TailCommentStrategy branch

          (test item has no comment)

        # test NextLine branch
        [TOP]
          sub-item
        '''
        dec = TestTop(FileContent('dummy', Content.splitlines()))
        dec.Parse()

        # Two sections
        self.assertEqual(len(dec.ItemObject), 2)

        data = dec.ItemObject[0]
        self.assertEqual(data._HeadComment[0][0], '# Top comment')
        self.assertEqual(data._HeadComment[0][1], 1)

        # 3 subitems
        self.assertEqual(len(data.ItemList), 3)

        dataitem = data.ItemList[0]
        self.assertEqual(dataitem.String, '(test item has both head and tail comment)')
        # Comment content
        self.assertEqual(dataitem._HeadComment[0][0], '# sub1 head comment')
        self.assertEqual(dataitem._TailComment[0][0], '# sub1 tail comment')
        # Comment line number
        self.assertEqual(dataitem._HeadComment[0][1], 3)
        self.assertEqual(dataitem._TailComment[0][1], 4)

        dataitem = data.ItemList[1]
        self.assertEqual(dataitem.String, '(test item has head and special tail comment)')
        # Comment content
        self.assertEqual(dataitem._HeadComment[0][0], '# sub2 head comment')
        self.assertEqual(dataitem._TailComment[0][0], '# @comment test TailCommentStrategy branch')
        # Comment line number
        self.assertEqual(dataitem._HeadComment[0][1], 5)
        self.assertEqual(dataitem._TailComment[0][1], 7)

        dataitem = data.ItemList[2]
        self.assertEqual(dataitem.String, '(test item has no comment)')
        # Comment content
        self.assertEqual(dataitem._HeadComment, [])
        self.assertEqual(dataitem._TailComment, [])

        data = dec.ItemObject[1]
        self.assertEqual(data._HeadComment[0][0], '# test NextLine branch')
        self.assertEqual(data._HeadComment[0][1], 11)

        # 1 subitems
        self.assertEqual(len(data.ItemList), 1)

        dataitem = data.ItemList[0]
        self.assertEqual(dataitem.String, 'sub-item')
        self.assertEqual(dataitem._HeadComment, [])
        self.assertEqual(dataitem._TailComment, [])

#
# Test _DecDefine._ParseItem
#
class DecDefineTestCase(unittest.TestCase):
    def GetObj(self, Content):
        Obj = _DecDefine(FileContent('dummy', Content.splitlines()))
        Obj._RawData.CurrentLine = Obj._RawData.GetNextLine()
        return Obj

    def testDecDefine(self):
        item = self.GetObj('PACKAGE_NAME = MdePkg')._ParseItem()
        self.assertEqual(item.Key, 'PACKAGE_NAME')
        self.assertEqual(item.Value, 'MdePkg')

    def testDecDefine1(self):
        obj = self.GetObj('PACKAGE_NAME')
        self.assertRaises(FatalError, obj._ParseItem)

    def testDecDefine2(self):
        obj = self.GetObj('unknown_key = ')
        self.assertRaises(FatalError, obj._ParseItem)

    def testDecDefine3(self):
        obj = self.GetObj('PACKAGE_NAME = ')
        self.assertRaises(FatalError, obj._ParseItem)

#
# Test _DecLibraryclass._ParseItem
#
class DecLibraryTestCase(unittest.TestCase):
    def GetObj(self, Content):
        Obj = _DecLibraryclass(FileContent('dummy', Content.splitlines()))
        Obj._RawData.CurrentLine = Obj._RawData.GetNextLine()
        return Obj

    def testNoInc(self):
        obj = self.GetObj('UefiRuntimeLib')
        self.assertRaises(FatalError, obj._ParseItem)

    def testEmpty(self):
        obj = self.GetObj(' | ')
        self.assertRaises(FatalError, obj._ParseItem)

    def testLibclassNaming(self):
        obj = self.GetObj('lowercase_efiRuntimeLib|Include/Library/UefiRuntimeLib.h')
        self.assertRaises(FatalError, obj._ParseItem)

    def testLibclassExt(self):
        obj = self.GetObj('RuntimeLib|Include/Library/UefiRuntimeLib.no_h')
        self.assertRaises(FatalError, obj._ParseItem)

    def testLibclassRelative(self):
        obj = self.GetObj('RuntimeLib|Include/../UefiRuntimeLib.h')
        self.assertRaises(FatalError, obj._ParseItem)

#
# Test _DecPcd._ParseItem
#
class DecPcdTestCase(unittest.TestCase):
    def GetObj(self, Content):
        Obj = _DecPcd(FileContent('dummy', Content.splitlines()))
        Obj._RawData.CurrentLine = Obj._RawData.GetNextLine()
        Obj._RawData.CurrentScope = [('PcdsFeatureFlag'.upper(), 'COMMON')]
        return Obj

    def testOK(self):
        item = self.GetObj('gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE|BOOLEAN|0x0000000d')._ParseItem()
        self.assertEqual(item.TokenSpaceGuidCName, 'gEfiMdePkgTokenSpaceGuid')
        self.assertEqual(item.TokenCName, 'PcdComponentNameDisable')
        self.assertEqual(item.DefaultValue, 'FALSE')
        self.assertEqual(item.DatumType, 'BOOLEAN')
        self.assertEqual(item.TokenValue, '0x0000000d')

    def testNoCvar(self):
        obj = self.GetObj('123ai.PcdComponentNameDisable|FALSE|BOOLEAN|0x0000000d')
        self.assertRaises(FatalError, obj._ParseItem)

    def testSplit(self):
        obj = self.GetObj('gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable FALSE|BOOLEAN|0x0000000d')
        self.assertRaises(FatalError, obj._ParseItem)

        obj = self.GetObj('gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE|BOOLEAN|0x0000000d | abc')
        self.assertRaises(FatalError, obj._ParseItem)

    def testUnknownType(self):
        obj = self.GetObj('gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE|unknown|0x0000000d')
        self.assertRaises(FatalError, obj._ParseItem)

    def testVoid(self):
        obj = self.GetObj('gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|abc|VOID*|0x0000000d')
        self.assertRaises(FatalError, obj._ParseItem)

    def testUINT(self):
        obj = self.GetObj('gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|0xabc|UINT8|0x0000000d')
        self.assertRaises(FatalError, obj._ParseItem)

#
# Test _DecInclude._ParseItem
#
class DecIncludeTestCase(unittest.TestCase):
    #
    # Test code to be added
    #
    pass

#
# Test _DecGuid._ParseItem
#
class DecGuidTestCase(unittest.TestCase):
    def GetObj(self, Content):
        Obj = _DecGuid(FileContent('dummy', Content.splitlines()))
        Obj._RawData.CurrentLine = Obj._RawData.GetNextLine()
        Obj._RawData.CurrentScope = [('guids'.upper(), 'COMMON')]
        return Obj

    def testCValue(self):
        item = self.GetObj('gEfiIpSecProtocolGuid={ 0xdfb386f7, 0xe100, 0x43ad,'
                           ' {0x9c, 0x9a, 0xed, 0x90, 0xd0, 0x8a, 0x5e, 0x12 }}')._ParseItem()
        self.assertEqual(item.GuidCName, 'gEfiIpSecProtocolGuid')
        self.assertEqual(item.GuidCValue, '{ 0xdfb386f7, 0xe100, 0x43ad, {0x9c, 0x9a, 0xed, 0x90, 0xd0, 0x8a, 0x5e, 0x12 }}')

    def testGuidString(self):
        item = self.GetObj('gEfiIpSecProtocolGuid=1E73767F-8F52-4603-AEB4-F29B510B6766')._ParseItem()
        self.assertEqual(item.GuidCName, 'gEfiIpSecProtocolGuid')
        self.assertEqual(item.GuidCValue, '1E73767F-8F52-4603-AEB4-F29B510B6766')

    def testNoValue1(self):
        obj = self.GetObj('gEfiIpSecProtocolGuid')
        self.assertRaises(FatalError, obj._ParseItem)

    def testNoValue2(self):
        obj = self.GetObj('gEfiIpSecProtocolGuid=')
        self.assertRaises(FatalError, obj._ParseItem)

    def testNoName(self):
        obj = self.GetObj('=')
        self.assertRaises(FatalError, obj._ParseItem)

#
# Test Dec.__init__
#
class DecDecInitTestCase(unittest.TestCase):
    def testNoDecFile(self):
        self.assertRaises(FatalError, Dec, 'No_Such_File')

class TmpFile:
    def __init__(self, File):
        self.File = File

    def Write(self, Content):
        try:
            FileObj = open(self.File, 'w')
            FileObj.write(Content)
            FileObj.close()
        except:
            pass

    def Remove(self):
        try:
            os.remove(self.File)
        except:
            pass

#
# Test Dec._UserExtentionSectionParser
#
class DecUESectionTestCase(unittest.TestCase):
    def setUp(self):
        self.File = TmpFile('test.dec')
        self.File.Write(
'''[userextensions.intel."myid"]
[userextensions.intel."myid".IA32]
[userextensions.intel."myid".IA32,]
[userextensions.intel."myid]
'''
        )

    def tearDown(self):
        self.File.Remove()

    def testUserExtentionHeader(self):
        dec = Dec('test.dec', False)

        # OK: [userextensions.intel."myid"]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        dec._UserExtentionSectionParser()
        self.assertEqual(len(dec._RawData.CurrentScope), 1)
        self.assertEqual(dec._RawData.CurrentScope[0][0], 'userextensions'.upper())
        self.assertEqual(dec._RawData.CurrentScope[0][1], 'intel')
        self.assertEqual(dec._RawData.CurrentScope[0][2], '"myid"')
        self.assertEqual(dec._RawData.CurrentScope[0][3], 'COMMON')

        # OK: [userextensions.intel."myid".IA32]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        dec._UserExtentionSectionParser()
        self.assertEqual(len(dec._RawData.CurrentScope), 1)
        self.assertEqual(dec._RawData.CurrentScope[0][0], 'userextensions'.upper())
        self.assertEqual(dec._RawData.CurrentScope[0][1], 'intel')
        self.assertEqual(dec._RawData.CurrentScope[0][2], '"myid"')
        self.assertEqual(dec._RawData.CurrentScope[0][3], 'IA32')

        # Fail: [userextensions.intel."myid".IA32,]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._UserExtentionSectionParser)

        # Fail: [userextensions.intel."myid]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._UserExtentionSectionParser)

#
# Test Dec._SectionHeaderParser
#
class DecSectionTestCase(unittest.TestCase):
    def setUp(self):
        self.File = TmpFile('test.dec')
        self.File.Write(
'''[no section start or end
[,] # empty sub-section
[unknow_section_name]
[Includes.IA32.other] # no third one
[PcdsFeatureFlag, PcdsFixedAtBuild] # feature flag PCD must not be in the same section of other types of PCD
[Includes.IA32, Includes.IA32]
[Includes, Includes.IA32] # common cannot be with other arch
[Includes.IA32, PcdsFeatureFlag] # different section name
'''     )

    def tearDown(self):
        self.File.Remove()

    def testSectionHeader(self):
        dec = Dec('test.dec', False)
        # [no section start or end
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

        #[,] # empty sub-section
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

        # [unknow_section_name]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

        # [Includes.IA32.other] # no third one
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

        # [PcdsFeatureFlag, PcdsFixedAtBuild]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

        # [Includes.IA32, Includes.IA32]
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        dec._SectionHeaderParser()
        self.assertEqual(len(dec._RawData.CurrentScope), 1)
        self.assertEqual(dec._RawData.CurrentScope[0][0], 'Includes'.upper())
        self.assertEqual(dec._RawData.CurrentScope[0][1], 'IA32')

        # [Includes, Includes.IA32] # common cannot be with other arch
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

        # [Includes.IA32, PcdsFeatureFlag] # different section name not allowed
        dec._RawData.CurrentLine = CleanString(dec._RawData.GetNextLine())[0]
        self.assertRaises(FatalError, dec._SectionHeaderParser)

#
# Test Dec._ParseDecComment
#
class DecDecCommentTestCase(unittest.TestCase):
    def testDecHeadComment(self):
        File = TmpFile('test.dec')
        File.Write(
       '''# abc
          ##''')
        dec = Dec('test.dec', False)
        dec.ParseDecComment()
        self.assertEqual(len(dec._HeadComment), 2)
        self.assertEqual(dec._HeadComment[0][0], '# abc')
        self.assertEqual(dec._HeadComment[0][1], 1)
        self.assertEqual(dec._HeadComment[1][0], '##')
        self.assertEqual(dec._HeadComment[1][1], 2)
        File.Remove()

    def testNoDoubleComment(self):
        File = TmpFile('test.dec')
        File.Write(
       '''# abc
          #
          [section_start]''')
        dec = Dec('test.dec', False)
        dec.ParseDecComment()
        self.assertEqual(len(dec._HeadComment), 2)
        self.assertEqual(dec._HeadComment[0][0], '# abc')
        self.assertEqual(dec._HeadComment[0][1], 1)
        self.assertEqual(dec._HeadComment[1][0], '#')
        self.assertEqual(dec._HeadComment[1][1], 2)
        File.Remove()

if __name__ == '__main__':
    import Logger.Logger
    Logger.Logger.Initialize()
    unittest.main()

