## @file
# Unit tests for Python based BaseTools
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
import sys
import unittest


def TheTestSuite():
    suites = []
    import CheckPythonSyntax
    suites.append(CheckPythonSyntax.TheTestSuite())
    return unittest.TestSuite(suites)

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)

