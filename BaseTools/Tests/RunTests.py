## @file
# Unit tests for BaseTools utilities
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

import TestTools

def GetCTestSuite():
    import CToolsTests
    return CToolsTests.TheTestSuite()

def GetPythonTestSuite():
    import PythonToolsTests
    return PythonToolsTests.TheTestSuite()

def GetAllTestsSuite():
    return unittest.TestSuite([GetCTestSuite(), GetPythonTestSuite()])

if __name__ == '__main__':
    allTests = GetAllTestsSuite()
    unittest.TextTestRunner(verbosity=2).run(allTests)

