# @file
# Unit tests for GenCrc32 utility
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import unittest
import TestTools


class Tests(TestTools.BaseToolsTest):

    def setUp(self):
        TestTools.BaseToolsTest.setUp(self)
        self.toolName = 'GenCrc32'

    def test_display_help(self):
        result = self.RunTool(
            '--help',
            logFile='help'
            )
        self.assertTrue(result == 0)

    def do_encode_decode(self, data):
        path = self.GetTmpFilePath('input')
        self.WriteTmpFile('input', data)
        result = self.RunTool(
            '--verbose',
            '-e',
            '-o', self.GetTmpFilePath('output1'),
            self.GetTmpFilePath('input'),
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
        start_equals_finish = start == finish
        if not start_equals_finish:
            print('Original data did not match decode(encode(data))')
            self.DisplayBinaryData('original data', start)
            self.DisplayBinaryData('after encoding',
                                   self.ReadTmpFile('output1'))
            self.DisplayBinaryData('after decoding', finish)
        self.assertTrue(start_equals_finish)

    def test_encode_decode_various_sizes(self):
        for i in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, 1000]:
            data = self.GetRandomString(i, i)
            self.do_encode_decode(data)
            self.CleanUpTmpDir()


TheTestSuite = TestTools.MakeTheTestSuite(locals())

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)
