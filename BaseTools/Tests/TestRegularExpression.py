## @file
# Routines for generating Pcd Database
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

import unittest
from Common.Misc import RemoveCComments
from Workspace.BuildClassObject import ArrayIndex

class TestRe(unittest.TestCase):
    def test_ccomments(self):
        TestStr1 = """ {0x01,0x02} """
        self.assertEquals(TestStr1, RemoveCComments(TestStr1))

        TestStr2 = """ L'TestString' """
        self.assertEquals(TestStr2, RemoveCComments(TestStr2))

        TestStr3 = """ 'TestString' """
        self.assertEquals(TestStr3, RemoveCComments(TestStr3))

        TestStr4 = """
            {CODE({
              {0x01, {0x02, 0x03, 0x04 }},// Data comment
              {0x01, {0x02, 0x03, 0x04 }},// Data comment
              })
            }  /*
               This is multiple line comments
               The seconde line comment
               */
            // This is a comment
        """
        Expect_TestStr4 = """{CODE({
              {0x01, {0x02, 0x03, 0x04 }},
              {0x01, {0x02, 0x03, 0x04 }},
              })
            }"""
        self.assertEquals(Expect_TestStr4, RemoveCComments(TestStr4).strip())

    def Test_ArrayIndex(self):
        TestStr1 = """[1]"""
        self.assertEquals(['[1]'], ArrayIndex.findall(TestStr1))

        TestStr2 = """[1][2][0x1][0x01][]"""
        self.assertEquals(['[1]','[2]','[0x1]','[0x01]','[]'], ArrayIndex.findall(TestStr2))

if __name__ == '__main__':
    unittest.main()
