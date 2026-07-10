## @file
# Unit tests for parsing the "## @RecommendedInstance" comment used in the
# [LibraryClasses] section of an EDK II INF file.
#
#  Copyright (c) Microsoft Corporation.
#  SPDX-License-Identifier: BSD-2-Clause-Patent

import os
import sys
import shutil
import tempfile
import unittest

#
# Make sure the BaseTools Python modules can be imported whether this test is
# run standalone, via pytest, or through the BaseTools test suite runner.
#
BaseToolsPythonDir = os.path.realpath(
    os.path.join(os.path.dirname(__file__), '..', 'Source', 'Python'))
if BaseToolsPythonDir not in sys.path:
    sys.path.insert(0, BaseToolsPythonDir)

import Common.GlobalData as GlobalData
from Common.Misc import PathClass
from CommonDataClass.DataClass import (
    MODEL_FILE_INF,
    MODEL_EFI_LIBRARY_CLASS,
)
from Workspace.MetaFileParser import InfParser, RecommendedInstancePattern
from Workspace.MetaFileTable import MetaFileStorage


class _FakeDB(object):
    """A minimal stand-in for the BaseTools workspace database.

    MetaFileTable only requires an object exposing a ``TblFile`` list, so the
    parser can be exercised without a real SQLite database.
    """

    def __init__(self):
        self.TblFile = []


class TestRecommendedInstancePattern(unittest.TestCase):
    """Tests for the RecommendedInstancePattern regular expression."""

    def test_double_hash_with_macro(self):
        Match = RecommendedInstancePattern.match(
            "## @RecommendedInstance $(MDE)/BaseDebugLibNull/BaseDebugLibNull.inf")
        self.assertIsNotNone(Match)
        self.assertEqual(
            Match.group(1), "$(MDE)/BaseDebugLibNull/BaseDebugLibNull.inf")

    def test_single_hash(self):
        Match = RecommendedInstancePattern.match(
            "#  @RecommendedInstance MdePkg/Library/BaseLib/BaseLib.inf")
        self.assertIsNotNone(Match)
        self.assertEqual(Match.group(1), "MdePkg/Library/BaseLib/BaseLib.inf")

    def test_trailing_whitespace_is_ignored(self):
        Match = RecommendedInstancePattern.match(
            "## @RecommendedInstance   MdePkg/Library/UefiLib/UefiLib.inf   ")
        self.assertIsNotNone(Match)
        self.assertEqual(Match.group(1), "MdePkg/Library/UefiLib/UefiLib.inf")

    def test_non_recommended_comment(self):
        self.assertIsNone(
            RecommendedInstancePattern.match("# Just a normal comment"))

    def test_missing_path(self):
        self.assertIsNone(
            RecommendedInstancePattern.match("## @RecommendedInstance"))


class TestRecommendedInstanceParsing(unittest.TestCase):
    """Tests that InfParser stores the recommended instance in the library
    class record's Value3 field."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        self._SavedWorkspace = GlobalData.gWorkspace
        GlobalData.gWorkspace = self.tmpdir

    def tearDown(self):
        GlobalData.gWorkspace = self._SavedWorkspace
        if os.path.exists(self.tmpdir):
            shutil.rmtree(self.tmpdir)

    def _ParseInf(self, Content):
        InfPath = os.path.join(self.tmpdir, "TestModule.inf")
        with open(InfPath, "w") as InfFile:
            InfFile.write(Content)

        MetaFile = PathClass(InfPath)
        Table = MetaFileStorage(_FakeDB(), MetaFile, MODEL_FILE_INF,
                                Temporary=False)
        Parser = InfParser(MetaFile, MODEL_FILE_INF, "COMMON", Table)
        Parser.Start()

        # Return a dict mapping library class name -> recommended instance
        # (Value3) for every parsed [LibraryClasses] record.
        RetVal = {}
        for Row in Table.CurrentContent:
            # Row layout: [ID, Model, Value1, Value2, Value3, Scope1, ...]
            if Row[0] >= 0 and Row[1] == MODEL_EFI_LIBRARY_CLASS:
                RetVal[Row[2]] = Row[4]
        return RetVal

    _INF_TEMPLATE = """## @file
# Test module
##

[Defines]
  INF_VERSION                    = 0x00010005
  BASE_NAME                      = TestModule
  FILE_GUID                      = 12345678-1234-1234-1234-123456789abc
  MODULE_TYPE                    = DXE_DRIVER
  VERSION_STRING                 = 1.0

[Sources]
  TestModule.c

[LibraryClasses]
{Body}
"""

    def _Parse(self, Body):
        return self._ParseInf(self._INF_TEMPLATE.format(Body=Body))

    def test_macro_is_resolved(self):
        Result = self._Parse(
            "  DEFINE MDE = MdePkg/Library\n"
            "  ## @RecommendedInstance $(MDE)/BaseDebugLibNull/BaseDebugLibNull.inf\n"
            "  DebugLib\n")
        self.assertEqual(
            Result.get("DebugLib"),
            "MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf")

    def test_no_recommended_instance(self):
        Result = self._Parse("  UefiDriverModelLib\n")
        self.assertEqual(Result.get("UefiDriverModelLib"), "")

    def test_comment_binds_to_following_class(self):
        # The @RecommendedInstance comment sits between two library classes
        # and must be associated with the one that follows it (BaseLib), not
        # the one that precedes it (UefiDriverModelLib).
        Result = self._Parse(
            "  UefiDriverModelLib\n"
            "  ## @RecommendedInstance MdePkg/Library/BaseLib/BaseLib.inf\n"
            "  BaseLib\n")
        self.assertEqual(Result.get("UefiDriverModelLib"), "")
        self.assertEqual(
            Result.get("BaseLib"), "MdePkg/Library/BaseLib/BaseLib.inf")

    def test_multiple_recommended_instances(self):
        Result = self._Parse(
            "  DEFINE MDE = MdePkg/Library\n"
            "  ## @RecommendedInstance $(MDE)/BaseDebugLibNull/BaseDebugLibNull.inf\n"
            "  DebugLib\n"
            "  UefiDriverModelLib\n"
            "  ## @RecommendedInstance MdePkg/Library/BaseLib/BaseLib.inf\n"
            "  BaseLib\n")
        self.assertEqual(
            Result.get("DebugLib"),
            "MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf")
        self.assertEqual(Result.get("UefiDriverModelLib"), "")
        self.assertEqual(
            Result.get("BaseLib"), "MdePkg/Library/BaseLib/BaseLib.inf")

    def test_ordinary_comment_is_ignored(self):
        Result = self._Parse(
            "  ## This is just a description, not a recommendation\n"
            "  DebugLib\n")
        self.assertEqual(Result.get("DebugLib"), "")


def TheTestSuite():
    suites = [
        unittest.TestLoader().loadTestsFromTestCase(TestRecommendedInstancePattern),
        unittest.TestLoader().loadTestsFromTestCase(TestRecommendedInstanceParsing),
    ]
    return unittest.TestSuite(suites)


if __name__ == '__main__':
    unittest.main()
