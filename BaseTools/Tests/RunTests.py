## @file
# Unit tests for BaseTools utilities
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
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

