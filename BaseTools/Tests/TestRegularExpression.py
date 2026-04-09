## @file
# Routines for generating Pcd Database
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

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
