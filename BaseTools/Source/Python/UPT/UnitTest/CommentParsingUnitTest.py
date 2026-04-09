## @file
# This file contain unit test for CommentParsing
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

import unittest

import Logger.Log as Logger
from Library.CommentParsing import ParseHeaderCommentSection, \
                                   ParseGenericComment, \
                                   ParseDecPcdGenericComment, \
                                   ParseDecPcdTailComment
from Library.CommentParsing import _IsCopyrightLine
from Library.StringUtils import GetSplitValueList
from Library.DataType import TAB_SPACE_SPLIT
from Library.DataType import TAB_LANGUAGE_EN_US

#
# Test ParseHeaderCommentSection
#
class ParseHeaderCommentSectionTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # Normal case1: have license/copyright/license above @file
    #
    def testNormalCase1(self):
        TestCommentLines1 = \
        '''# License1
        # License2
        #
        ## @file
        # example abstract
        #
        # example description
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        #
        # License3
        #'''

        CommentList = GetSplitValueList(TestCommentLines1, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = 'example abstract'
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = 'example description'
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2010,'\
            ' Intel Corporation. All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = 'License1\nLicense2\n\nLicense3'
        self.assertEqual(License, ExpectedLicense)

    #
    # Normal case2: have license/copyright above @file, but no copyright after
    #
    def testNormalCase2(self):
        TestCommentLines2 = \
        ''' # License1
        # License2
        #
        ## @file
        # example abstract
        #
        # example description
        #
        #Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines2, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = 'example abstract'
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = 'example description'
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = 'License1\nLicense2'
        self.assertEqual(License, ExpectedLicense)


    #
    # Normal case2: have license/copyright/license above @file,
    # but no abstract/description
    #
    def testNormalCase3(self):
        TestCommentLines3 = \
        ''' # License1
        # License2
        #
        ## @file
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        #
        # License3 Line1
        # License3 Line2
        ##'''

        CommentList = GetSplitValueList(TestCommentLines3, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = ''
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = ''
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2010,'\
            ' Intel Corporation. All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = \
            'License1\n' \
            'License2\n\n' \
            'License3 Line1\n' \
            'License3 Line2'
        self.assertEqual(License, ExpectedLicense)

    #
    # Normal case4: format example in spec
    #
    def testNormalCase4(self):
        TestCommentLines = \
        '''
        ## @file
        # Abstract
        #
        # Description
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        #
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = 'Abstract'
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = 'Description'
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = \
            'License'
        self.assertEqual(License, ExpectedLicense)

    #
    # Normal case5: other line between copyright
    #
    def testNormalCase5(self):
        TestCommentLines = \
        '''
        ## @file
        # Abstract
        #
        # Description
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        # other line
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        #
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = 'Abstract'
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = 'Description'
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>\n'\
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = \
            'License'
        self.assertEqual(License, ExpectedLicense)

    #
    # Normal case6: multiple lines of copyright
    #
    def testNormalCase6(self):
        TestCommentLines = \
        '''
        ## @file
        # Abstract
        #
        # Description
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        # Copyright (c) 2007 - 2010, FOO1 Corporation. All rights reserved.<BR>
        # Copyright (c) 2007 - 2010, FOO2 Corporation. All rights reserved.<BR>
        #
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = 'Abstract'
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = 'Description'
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>\n'\
            'Copyright (c) 2007 - 2010, FOO1 Corporation.'\
            ' All rights reserved.<BR>\n'\
            'Copyright (c) 2007 - 2010, FOO2 Corporation.'\
            ' All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = \
            'License'
        self.assertEqual(License, ExpectedLicense)

    #
    # Normal case7: Abstract not present
    #
    def testNormalCase7(self):
        TestCommentLines = \
        '''
        ## @file
        #
        # Description
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        # Copyright (c) 2007 - 2010, FOO1 Corporation. All rights reserved.<BR>
        # Copyright (c) 2007 - 2010, FOO2 Corporation. All rights reserved.<BR>
        #
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = ''
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = 'Description'
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>\n'\
            'Copyright (c) 2007 - 2010, FOO1 Corporation.'\
            ' All rights reserved.<BR>\n'\
            'Copyright (c) 2007 - 2010, FOO2 Corporation.'\
            ' All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = \
            'License'
        self.assertEqual(License, ExpectedLicense)

    #
    # Normal case8: Description not present
    #
    def testNormalCase8(self):
        TestCommentLines = \
        '''
        ## @file
        # Abstact
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        #
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(TestCommentLinesList, "PhonyFile")

        ExpectedAbstract = 'Abstact'
        self.assertEqual(Abstract, ExpectedAbstract)

        ExpectedDescription = ''
        self.assertEqual(Description, ExpectedDescription)

        ExpectedCopyright = \
            'Copyright (c) 2007 - 2018, Intel Corporation.'\
            ' All rights reserved.<BR>'
        self.assertEqual(Copyright, ExpectedCopyright)

        ExpectedLicense = \
            'License'
        self.assertEqual(License, ExpectedLicense)

    #
    # Error case1: No copyright found
    #
    def testErrorCase1(self):
        TestCommentLines = \
        '''
        ## @file
        # Abstract
        #
        # Description
        #
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        self.assertRaises(Logger.FatalError,
                          ParseHeaderCommentSection,
                          TestCommentLinesList,
                          "PhonyFile")

    #
    # Error case2: non-empty non-comment lines passed in
    #
    def testErrorCase2(self):
        TestCommentLines = \
        '''
        ## @file
        # Abstract
        #
        this is invalid line
        # Description
        #
        # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
        # License
        #
        ##'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        self.assertRaises(Logger.FatalError,
                          ParseHeaderCommentSection,
                          TestCommentLinesList,
                          "PhonyFile")

#
# Test ParseGenericComment
#
class ParseGenericCommentTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # Normal case1: one line of comment
    #
    def testNormalCase1(self):
        TestCommentLines = \
        '''# hello world'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        HelptxtObj = ParseGenericComment(TestCommentLinesList, 'testNormalCase1')
        self.failIf(not HelptxtObj)
        self.assertEqual(HelptxtObj.GetString(), 'hello world')
        self.assertEqual(HelptxtObj.GetLang(), TAB_LANGUAGE_EN_US)

    #
    # Normal case2: multiple lines of comment
    #
    def testNormalCase2(self):
        TestCommentLines = \
        '''## hello world
        # second line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        HelptxtObj = ParseGenericComment(TestCommentLinesList, 'testNormalCase2')
        self.failIf(not HelptxtObj)
        self.assertEqual(HelptxtObj.GetString(),
                         'hello world\n' + 'second line')
        self.assertEqual(HelptxtObj.GetLang(), TAB_LANGUAGE_EN_US)

    #
    # Normal case3: multiple lines of comment, non comment lines will be skipped
    #
    def testNormalCase3(self):
        TestCommentLines = \
        '''## hello world
        This is not comment line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        HelptxtObj = ParseGenericComment(TestCommentLinesList, 'testNormalCase3')
        self.failIf(not HelptxtObj)
        self.assertEqual(HelptxtObj.GetString(),
                         'hello world\n\n')
        self.assertEqual(HelptxtObj.GetLang(), TAB_LANGUAGE_EN_US)

#
# Test ParseDecPcdGenericComment
#
class ParseDecPcdGenericCommentTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # Normal case1: comments with no special comment
    #
    def testNormalCase1(self):
        TestCommentLines = \
        '''## hello world
        # second line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (HelpTxt, PcdErr) = \
            ParseDecPcdGenericComment(TestCommentLinesList, 'testNormalCase1')
        self.failIf(not HelpTxt)
        self.failIf(PcdErr)
        self.assertEqual(HelpTxt,
                         'hello world\n' + 'second line')


    #
    # Normal case2: comments with valid list
    #
    def testNormalCase2(self):
        TestCommentLines = \
        '''## hello world
        # second line
        # @ValidList 1, 2, 3
        # other line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (HelpTxt, PcdErr) = \
            ParseDecPcdGenericComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpTxt)
        self.failIf(not PcdErr)
        self.assertEqual(HelpTxt,
                         'hello world\n' + 'second line\n' + 'other line')
        ExpectedList = GetSplitValueList('1 2 3', TAB_SPACE_SPLIT)
        ActualList = [item for item in \
            GetSplitValueList(PcdErr.GetValidValue(), TAB_SPACE_SPLIT) if item]
        self.assertEqual(ExpectedList, ActualList)
        self.failIf(PcdErr.GetExpression())
        self.failIf(PcdErr.GetValidValueRange())

    #
    # Normal case3: comments with valid range
    #
    def testNormalCase3(self):
        TestCommentLines = \
        '''## hello world
        # second line
        # @ValidRange LT 1 AND GT 2
        # other line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (HelpTxt, PcdErr) = \
            ParseDecPcdGenericComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpTxt)
        self.failIf(not PcdErr)
        self.assertEqual(HelpTxt,
                         'hello world\n' + 'second line\n' + 'other line')
        self.assertEqual(PcdErr.GetValidValueRange().strip(), 'LT 1 AND GT 2')
        self.failIf(PcdErr.GetExpression())
        self.failIf(PcdErr.GetValidValue())

    #
    # Normal case4: comments with valid expression
    #
    def testNormalCase4(self):
        TestCommentLines = \
        '''## hello world
        # second line
        # @Expression LT 1 AND GT 2
        # other line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (HelpTxt, PcdErr) = \
            ParseDecPcdGenericComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpTxt)
        self.failIf(not PcdErr)
        self.assertEqual(HelpTxt,
                         'hello world\n' + 'second line\n' + 'other line')
        self.assertEqual(PcdErr.GetExpression().strip(), 'LT 1 AND GT 2')
        self.failIf(PcdErr.GetValidValueRange())
        self.failIf(PcdErr.GetValidValue())

    #
    # Normal case5: comments with valid expression and no generic comment
    #
    def testNormalCase5(self):
        TestCommentLines = \
        '''# @Expression LT 1 AND GT 2'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (HelpTxt, PcdErr) = \
            ParseDecPcdGenericComment(TestCommentLinesList, 'UnitTest')
        self.failIf(HelpTxt)
        self.failIf(not PcdErr)
        self.assertEqual(PcdErr.GetExpression().strip(), 'LT 1 AND GT 2')
        self.failIf(PcdErr.GetValidValueRange())
        self.failIf(PcdErr.GetValidValue())

    #
    # Normal case6: comments with only generic help text
    #
    def testNormalCase6(self):
        TestCommentLines = \
        '''#'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (HelpTxt, PcdErr) = \
            ParseDecPcdGenericComment(TestCommentLinesList, 'UnitTest')
        self.assertEqual(HelpTxt, '\n')
        self.failIf(PcdErr)



    #
    # Error case1: comments with both expression and valid list, use later
    # ignore the former and with a warning message
    #
    def testErrorCase1(self):
        TestCommentLines = \
        '''## hello world
        # second line
        # @ValidList 1, 2, 3
        # @Expression LT 1 AND GT 2
        # other line'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        try:
            ParseDecPcdGenericComment(TestCommentLinesList, 'UnitTest')
        except Logger.FatalError:
            pass

#
# Test ParseDecPcdTailComment
#
class ParseDecPcdTailCommentTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # Normal case1: comments with no SupModeList
    #
    def testNormalCase1(self):
        TestCommentLines = \
        '''## #hello world'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (SupModeList, HelpStr) = \
            ParseDecPcdTailComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpStr)
        self.failIf(SupModeList)
        self.assertEqual(HelpStr,
                         'hello world')

    #
    # Normal case2: comments with one SupMode
    #
    def testNormalCase2(self):
        TestCommentLines = \
        '''## BASE #hello world'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (SupModeList, HelpStr) = \
            ParseDecPcdTailComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpStr)
        self.failIf(not SupModeList)
        self.assertEqual(HelpStr,
                         'hello world')
        self.assertEqual(SupModeList,
                         ['BASE'])

    #
    # Normal case3: comments with more than one SupMode
    #
    def testNormalCase3(self):
        TestCommentLines = \
        '''## BASE  UEFI_APPLICATION #hello world'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (SupModeList, HelpStr) = \
            ParseDecPcdTailComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpStr)
        self.failIf(not SupModeList)
        self.assertEqual(HelpStr,
                         'hello world')
        self.assertEqual(SupModeList,
                         ['BASE', 'UEFI_APPLICATION'])

    #
    # Normal case4: comments with more than one SupMode, no help text
    #
    def testNormalCase4(self):
        TestCommentLines = \
        '''## BASE  UEFI_APPLICATION'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (SupModeList, HelpStr) = \
            ParseDecPcdTailComment(TestCommentLinesList, 'UnitTest')
        self.failIf(HelpStr)
        self.failIf(not SupModeList)
        self.assertEqual(SupModeList,
                         ['BASE', 'UEFI_APPLICATION'])

    #
    # Normal case5: general comments with no supModList, extract from real case
    #
    def testNormalCase5(self):
        TestCommentLines = \
        ''' # 1 = 128MB, 2 = 256MB, 3 = MAX'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        (SupModeList, HelpStr) = \
            ParseDecPcdTailComment(TestCommentLinesList, 'UnitTest')
        self.failIf(not HelpStr)
        self.assertEqual(HelpStr,
                         '1 = 128MB, 2 = 256MB, 3 = MAX')
        self.failIf(SupModeList)


    #
    # Error case2: comments with supModList contains valid and invalid
    # module type
    #
    def testErrorCase2(self):
        TestCommentLines = \
        '''## BASE INVALID_MODULE_TYPE #hello world'''

        CommentList = GetSplitValueList(TestCommentLines, "\n")
        LineNum = 0
        TestCommentLinesList = []
        for Comment in CommentList:
            LineNum += 1
            TestCommentLinesList.append((Comment, LineNum))

        try:
            ParseDecPcdTailComment(TestCommentLinesList, 'UnitTest')
        except Logger.FatalError:
            pass


#
# Test _IsCopyrightLine
#
class _IsCopyrightLineTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    #
    # Normal case
    #
    def testCase1(self):
        Line = 'this is a copyright ( line'
        Result = _IsCopyrightLine(Line)
        self.failIf(not Result)

    #
    # Normal case
    #
    def testCase2(self):
        Line = 'this is a Copyright ( line'
        Result = _IsCopyrightLine(Line)
        self.failIf(not Result)

    #
    # Normal case
    #
    def testCase3(self):
        Line = 'this is not aCopyright ( line'
        Result = _IsCopyrightLine(Line)
        self.failIf(Result)

    #
    # Normal case
    #
    def testCase4(self):
        Line = 'this is Copyright( line'
        Result = _IsCopyrightLine(Line)
        self.failIf(not Result)

    #
    # Normal case
    #
    def testCase5(self):
        Line = 'this is Copyright         (line'
        Result = _IsCopyrightLine(Line)
        self.failIf(not Result)

    #
    # Normal case
    #
    def testCase6(self):
        Line = 'this is not Copyright line'
        Result = _IsCopyrightLine(Line)
        self.failIf(Result)

    #
    # Normal case
    #
    def testCase7(self):
        Line = 'Copyright (c) line'
        Result = _IsCopyrightLine(Line)
        self.failIf(not Result)

    #
    # Normal case
    #
    def testCase8(self):
        Line = ' Copyright (c) line'
        Result = _IsCopyrightLine(Line)
        self.failIf(not Result)

    #
    # Normal case
    #
    def testCase9(self):
        Line = 'not a Copyright '
        Result = _IsCopyrightLine(Line)
        self.failIf(Result)

if __name__ == '__main__':
    Logger.Initialize()
    unittest.main()
