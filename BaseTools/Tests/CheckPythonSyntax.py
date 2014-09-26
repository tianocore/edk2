## @file
#  Unit tests for checking syntax of Python source code
#
#  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
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
import py_compile

import TestTools

class Tests(TestTools.BaseToolsTest):

    def setUp(self):
        TestTools.BaseToolsTest.setUp(self)

    def SingleFileTest(self, filename):
        try:
            py_compile.compile(filename, doraise=True)
        except Exception, e:
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


