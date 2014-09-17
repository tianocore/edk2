## @file
# Unit tests for TianoCompress utility
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
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
import random
import sys
import unittest

import TestTools

class Tests(TestTools.BaseToolsTest):

    def setUp(self):
        TestTools.BaseToolsTest.setUp(self)
        self.toolName = 'TianoCompress'

    def testHelp(self):
        result = self.RunTool('--help', logFile='help')
        #self.DisplayFile('help')
        self.assertTrue(result == 0)

    def compressionTestCycle(self, data):
        path = self.GetTmpFilePath('input')
        self.WriteTmpFile('input', data)
        result = self.RunTool(
            '-e',
            '-o', self.GetTmpFilePath('output1'),
            self.GetTmpFilePath('input')
            )
        self.assertTrue(result == 0)
        result = self.RunTool(
            '-d',
            '-o', self.GetTmpFilePath('output2'),
            self.GetTmpFilePath('output1')
            )
        self.assertTrue(result == 0)
        start = self.ReadTmpFile('input')
        finish = self.ReadTmpFile('output2')
        startEqualsFinish = start == finish
        if not startEqualsFinish:
            print
            print 'Original data did not match decompress(compress(data))'
            self.DisplayBinaryData('original data', start)
            self.DisplayBinaryData('after compression', self.ReadTmpFile('output1'))
            self.DisplayBinaryData('after decomression', finish)
        self.assertTrue(startEqualsFinish)

    def testRandomDataCycles(self):
        for i in range(8):
            data = self.GetRandomString(1024, 2048)
            self.compressionTestCycle(data)
            self.CleanUpTmpDir()

TheTestSuite = TestTools.MakeTheTestSuite(locals())

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)


