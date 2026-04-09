## @file
# Unit tests for Python based BaseTools
#
#  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
import os
import sys
import unittest


def TheTestSuite():
    suites = []
    import CheckPythonSyntax
    suites.append(CheckPythonSyntax.TheTestSuite())
    import CheckUnicodeSourceFiles
    suites.append(CheckUnicodeSourceFiles.TheTestSuite())
    return unittest.TestSuite(suites)

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)

