## @file
#  Unit tests for AutoGen.UniClassObject
#
#  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
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
import os
import unittest

import codecs

import TestTools

from Common.Misc import PathClass
import AutoGen.UniClassObject as BtUni

from Common import EdkLogger
EdkLogger.InitializeForUnitTest()

class Tests(TestTools.BaseToolsTest):

    SampleData = u'''
        #langdef en-US "English"
        #string STR_A #language en-US "STR_A for en-US"
    '''

    def EncodeToFile(self, encoding, string=None):
        if string is None:
            string = self.SampleData
        if encoding is not None:
            data = codecs.encode(string, encoding)
        else:
            data = string
        path = 'input.uni'
        self.WriteTmpFile(path, data)
        return PathClass(self.GetTmpFilePath(path))

    def ErrorFailure(self, error, encoding, shouldPass):
        msg = error + ' should '
        if shouldPass:
            msg += 'not '
        msg += 'be generated for '
        msg += '%s data in a .uni file' % encoding
        self.fail(msg)

    def UnicodeErrorFailure(self, encoding, shouldPass):
        self.ErrorFailure('UnicodeError', encoding, shouldPass)

    def EdkErrorFailure(self, encoding, shouldPass):
        self.ErrorFailure('EdkLogger.FatalError', encoding, shouldPass)

    def CheckFile(self, encoding, shouldPass, string=None):
        path = self.EncodeToFile(encoding, string)
        try:
            BtUni.UniFileClassObject([path])
            if shouldPass:
                return
        except UnicodeError:
            if not shouldPass:
                return
            else:
                self.UnicodeErrorFailure(encoding, shouldPass)
        except EdkLogger.FatalError:
            if not shouldPass:
                return
            else:
                self.EdkErrorFailure(encoding, shouldPass)
        except Exception:
            pass

        self.EdkErrorFailure(encoding, shouldPass)

    def testUtf16InUniFile(self):
        self.CheckFile('utf_16', shouldPass=True)

    def testSupplementaryPlaneUnicodeCharInUtf16File(self):
        #
        # Supplementary Plane characters can exist in UTF-16 files,
        # but they are not valid UCS-2 characters.
        #
        # This test makes sure that BaseTools rejects these characters
        # if seen in a .uni file.
        #
        data = u'''
            #langdef en-US "English"
            #string STR_A #language en-US "CodePoint (\U00010300) > 0xFFFF"
        '''

        self.CheckFile('utf_16', shouldPass=False, string=data)

    def testSurrogatePairUnicodeCharInUtf16File(self):
        #
        # Surrogate Pair code points are used in UTF-16 files to
        # encode the Supplementary Plane characters. But, a Surrogate
        # Pair code point which is not followed by another Surrogate
        # Pair code point might be interpreted as a single code point
        # with the Surrogate Pair code point.
        #
        # This test makes sure that BaseTools rejects these characters
        # if seen in a .uni file.
        #
        data = codecs.BOM_UTF16_LE + '//\x01\xd8 '

        self.CheckFile(encoding=None, shouldPass=False, string=data)

    def testValidUtf8File(self):
        self.CheckFile(encoding='utf_8', shouldPass=True)

    def testValidUtf8FileWithBom(self):
        #
        # Same test as testValidUtf8File, but add the UTF-8 BOM
        #
        data = codecs.BOM_UTF8 + codecs.encode(self.SampleData, 'utf_8')

        self.CheckFile(encoding=None, shouldPass=True, string=data)

    def test32bitUnicodeCharInUtf8File(self):
        data = u'''
            #langdef en-US "English"
            #string STR_A #language en-US "CodePoint (\U00010300) > 0xFFFF"
        '''

        self.CheckFile('utf_16', shouldPass=False, string=data)

    def test32bitUnicodeCharInUtf8File(self):
        data = u'''
            #langdef en-US "English"
            #string STR_A #language en-US "CodePoint (\U00010300) > 0xFFFF"
        '''

        self.CheckFile('utf_8', shouldPass=False, string=data)

    def test32bitUnicodeCharInUtf8Comment(self):
        data = u'''
            // Even in comments, we reject non-UCS-2 chars: \U00010300
            #langdef en-US "English"
            #string STR_A #language en-US "A"
        '''

        self.CheckFile('utf_8', shouldPass=False, string=data)

    def testSurrogatePairUnicodeCharInUtf8File(self):
        #
        # Surrogate Pair code points are used in UTF-16 files to
        # encode the Supplementary Plane characters. In UTF-8, it is
        # trivial to encode these code points, but they are not valid
        # code points for characters, since they are reserved for the
        # UTF-16 Surrogate Pairs.
        #
        # This test makes sure that BaseTools rejects these characters
        # if seen in a .uni file.
        #
        data = '\xed\xa0\x81'

        self.CheckFile(encoding=None, shouldPass=False, string=data)

    def testSurrogatePairUnicodeCharInUtf8FileWithBom(self):
        #
        # Same test as testSurrogatePairUnicodeCharInUtf8File, but add
        # the UTF-8 BOM
        #
        data = codecs.BOM_UTF8 + '\xed\xa0\x81'

        self.CheckFile(encoding=None, shouldPass=False, string=data)

TheTestSuite = TestTools.MakeTheTestSuite(locals())

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)
