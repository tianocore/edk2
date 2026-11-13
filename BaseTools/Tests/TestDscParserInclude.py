## @file
# Unit tests for DSC !include parsing
#
# Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import unittest

import TestTools
from Common import GlobalData
from Common.DataType import TAB_ARCH_COMMON
from Common.Misc import PathClass
from CommonDataClass.DataClass import (
    MODEL_EFI_LIBRARY_CLASS,
    MODEL_FILE_DSC,
    MODEL_META_DATA_COMPONENT,
)
from Workspace.MetaFileParser import DscParser
from Workspace.MetaFileTable import MetaFileStorage
from Workspace.WorkspaceDatabase import WorkspaceDatabase


class TestDscParserInclude(TestTools.BaseToolsTest):
    def setUp(self):
        super().setUp()
        self.savedGlobalDefines = GlobalData.gGlobalDefines.copy()
        self.savedPlatformDefines = GlobalData.gPlatformDefines
        self.savedPlatformOtherPcds = GlobalData.gPlatformOtherPcds.copy()
        self.savedWorkspace = GlobalData.gWorkspace

        GlobalData.gGlobalDefines['WORKSPACE'] = self.testDir
        GlobalData.gWorkspace = self.testDir

    def tearDown(self):
        GlobalData.gGlobalDefines.clear()
        GlobalData.gGlobalDefines.update(self.savedGlobalDefines)
        GlobalData.gPlatformDefines = self.savedPlatformDefines
        GlobalData.gPlatformOtherPcds.clear()
        GlobalData.gPlatformOtherPcds.update(self.savedPlatformOtherPcds)
        GlobalData.gWorkspace = self.savedWorkspace
        super().tearDown()

    def test_included_section_applies_to_following_parent_content(self):
        self.WriteTmpFile(
            'Included.dsc.inc',
            '[Components]\n'
            '  Included.inf\n'
            '[LibraryClasses]\n'
            '  IncludedLibrary|IncludedLibrary.inf\n'
            '[Components]\n',
        )
        self.WriteTmpFile(
            'LibraryClasses.dsc.inc',
            '[LibraryClasses]\n'
            '  SecondIncludedLibrary|SecondIncludedLibrary.inf\n',
        )
        self.WriteTmpFile(
            'Platform.dsc',
            '[LibraryClasses]\n'
            '  BeforeInclude|Before.inf\n'
            '!include Included.dsc.inc\n'
            '  AfterInclude|After.inf\n'
            '!include LibraryClasses.dsc.inc\n'
            '  AfterSecondInclude|AfterSecond.inf\n'
            '[Components]\n'
            '  AfterNextSection.inf\n',
        )

        DscFile = PathClass(os.path.join(self.testDir, 'Platform.dsc'))
        Table = MetaFileStorage(
            WorkspaceDatabase(),
            DscFile,
            MODEL_FILE_DSC,
            True,
        )
        Parser = DscParser(DscFile, MODEL_FILE_DSC, TAB_ARCH_COMMON, Table)

        LibraryClasses = Parser[MODEL_EFI_LIBRARY_CLASS, TAB_ARCH_COMMON]
        Components = Parser[MODEL_META_DATA_COMPONENT, TAB_ARCH_COMMON]
        LibraryClassRecords = [(Record[0], Record[1]) for Record in LibraryClasses]
        ComponentRecords = [Record[0] for Record in Components]

        self.assertIn(('BeforeInclude', 'Before.inf'), LibraryClassRecords)
        self.assertNotIn(('AfterInclude', 'After.inf'), LibraryClassRecords)
        self.assertIn(('IncludedLibrary', 'IncludedLibrary.inf'), LibraryClassRecords)
        self.assertIn(
            ('SecondIncludedLibrary', 'SecondIncludedLibrary.inf'),
            LibraryClassRecords,
        )
        self.assertIn(('AfterSecondInclude', 'AfterSecond.inf'), LibraryClassRecords)
        self.assertIn('AfterInclude|After.inf', ComponentRecords)
        self.assertIn('Included.inf', ComponentRecords)
        self.assertIn('AfterNextSection.inf', ComponentRecords)
        self.assertNotIn('AfterSecondInclude|AfterSecond.inf', ComponentRecords)


def TheTestSuite():
    return unittest.TestLoader().loadTestsFromTestCase(TestDscParserInclude)


if __name__ == '__main__':
    unittest.TextTestRunner(verbosity=2).run(TheTestSuite())
