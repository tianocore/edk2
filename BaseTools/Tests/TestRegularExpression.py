## @file
# Routines for generating Pcd Database
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

import unittest
from Common.DataType import TAB_VOID
from Common.Expression import ValueExpression, ValueExpressionEx
from Common.Misc import RemoveCComments
from Workspace.BuildClassObject import ArrayIndex


class TestValueExpressionEx(unittest.TestCase):
    def test_simple_byte_array_matches_legacy_parser(self):
        value = ' { 0X01, 0xaB, 0xff } '

        self.assertEqual(ValueExpression(value)(True), ValueExpressionEx(value, TAB_VOID)(True))

    def test_nested_simple_byte_array_uses_legacy_parser(self):
        value = '{ 0X01, 0x02 }'

        self.assertEqual(ValueExpression(value)(True, 1), ValueExpressionEx(value, TAB_VOID)(True, 1))

    def test_multiline_byte_array_uses_legacy_parser(self):
        value = '{0x01,\n0x02}'

        self.assertEqual('{0x01, 0x02}', ValueExpressionEx(value, TAB_VOID)(True))

    def test_non_dsc_whitespace_uses_legacy_parser(self):
        value = '{0x01,\v0x02}'

        self.assertEqual('{0x01, 0x02}', ValueExpressionEx(value, TAB_VOID)(True))

    def test_structured_array_uses_legacy_parser(self):
        value = '{UINT16(0x1234), 0x56}'

        self.assertEqual('{0x34, 0x12, 0x56}', ValueExpressionEx(value, TAB_VOID)(True))

    def test_trailing_comma_uses_legacy_parser(self):
        value = '{0x01,}'

        self.assertEqual('{0x01}', ValueExpressionEx(value, TAB_VOID)(True))

class TestRe(unittest.TestCase):
    def test_ccomments(self):
        TestStr1 = """ {0x01,0x02} """
        self.assertEqual(TestStr1, RemoveCComments(TestStr1))

        TestStr2 = """ L'TestString' """
        self.assertEqual(TestStr2, RemoveCComments(TestStr2))

        TestStr3 = """ 'TestString' """
        self.assertEqual(TestStr3, RemoveCComments(TestStr3))

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
        self.assertEqual(Expect_TestStr4, RemoveCComments(TestStr4).strip())

    def Test_ArrayIndex(self):
        TestStr1 = """[1]"""
        self.assertEqual(['[1]'], ArrayIndex.findall(TestStr1))

        TestStr2 = """[1][2][0x1][0x01][]"""
        self.assertEqual(['[1]','[2]','[0x1]','[0x01]','[]'], ArrayIndex.findall(TestStr2))

if __name__ == '__main__':
    unittest.main()
