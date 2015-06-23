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
        data = codecs.encode(string, encoding)
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

TheTestSuite = TestTools.MakeTheTestSuite(locals())

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)
