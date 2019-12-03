## @file
#  Unit tests for checking syntax of Python source code
#
#  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
import os
import unittest
import py_compile

import TestTools

class Tests(TestTools.BaseToolsTest):

    def setUp(self):
        TestTools.BaseToolsTest.setUp(self)

    def SingleFileTest(self, filename):
        try:
            py_compile.compile(filename, doraise=True)
        except Exception as e:
            self.fail('syntax error: %s, Error is %s' % (filename, str(e)))

def MakePythonSyntaxCheckTests():
    def GetAllPythonSourceFiles():
        pythonSourceFiles = []
        for (root, dirs, files) in os.walk(TestTools.PythonSourceDir):
            for filename in files:
                if filename.lower().endswith('.py'):
                    pythonSourceFiles.append(
                            os.path.join(root, filename)
                        )
        return pythonSourceFiles

    def MakeTestName(filename):
        assert filename.lower().endswith('.py')
        name = filename[:-3]
        name = name.replace(TestTools.PythonSourceDir, '')
        name = name.replace(os.path.sep, '_')
        return 'test' + name

    def MakeNewTest(filename):
        test = MakeTestName(filename)
        newmethod = lambda self: self.SingleFileTest(filename)
        setattr(
            Tests,
            test,
            newmethod
            )

    for filename in GetAllPythonSourceFiles():
        MakeNewTest(filename)

MakePythonSyntaxCheckTests()
del MakePythonSyntaxCheckTests

TheTestSuite = TestTools.MakeTheTestSuite(locals())

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)


