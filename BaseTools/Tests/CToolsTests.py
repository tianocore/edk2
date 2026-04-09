## @file
# Unit tests for C based BaseTools
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

import TianoCompress
modules = (
    TianoCompress,
    )


def TheTestSuite():
    suites = list(map(lambda module: module.TheTestSuite(), modules))
    return unittest.TestSuite(suites)

if __name__ == '__main__':
    allTests = TheTestSuite()
    unittest.TextTestRunner().run(allTests)

