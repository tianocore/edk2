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
from Common.DataType import TAB_ARCH_COMMON, TAB_ARCH_RISCV64
from Common.Misc import PathClass
from CommonDataClass.DataClass import (
    MODEL_EFI_LIBRARY_CLASS,
    MODEL_FILE_DSC,
    MODEL_META_DATA_COMPONENT,
    MODEL_META_DATA_HEADER,
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
            '!include Nested.dsc.inc\n',
        )
        self.WriteTmpFile(
            'Nested.dsc.inc',
            '[Components]\n'
            '  Nested.inf\n',
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
        self.assertIn('Nested.inf', ComponentRecords)
        self.assertIn('AfterNextSection.inf', ComponentRecords)
        self.assertNotIn('AfterSecondInclude|AfterSecond.inf', ComponentRecords)

    def test_consecutive_includes_use_second_section_for_parent_tail(self):
        self.WriteTmpFile(
            'First.dsc.inc',
            '[LibraryClasses]\n'
            '  FirstLibrary|FirstLibrary.inf\n',
        )
        self.WriteTmpFile(
            'Second.dsc.inc',
            '[Components.IA32, Components.X64, Components.RISCV64, Components.LOONGARCH64]\n'
            '  Second.inf\n',
        )
        self.WriteTmpFile(
            'Platform.dsc',
            '[Defines]\n'
            '  PLATFORM_NAME = Platform\n'
            '!include First.dsc.inc\n'
            '!include Second.dsc.inc\n'
            '  POSTBUILD = python GenerateCapsule.py\n',
        )

        DscFile = PathClass(os.path.join(self.testDir, 'Platform.dsc'))
        Table = MetaFileStorage(
            WorkspaceDatabase(),
            DscFile,
            MODEL_FILE_DSC,
            True,
        )
        Parser = DscParser(DscFile, MODEL_FILE_DSC, TAB_ARCH_RISCV64, Table)

        Headers = Parser[MODEL_META_DATA_HEADER, TAB_ARCH_RISCV64]
        LibraryClasses = Parser[MODEL_EFI_LIBRARY_CLASS, TAB_ARCH_RISCV64]
        Components = Parser[MODEL_META_DATA_COMPONENT, TAB_ARCH_RISCV64]
        HeaderRecords = [(Record[0], Record[1]) for Record in Headers]
        LibraryClassRecords = [(Record[0], Record[1]) for Record in LibraryClasses]
        ComponentRecords = [Record[0] for Record in Components]

        self.assertIn(('FirstLibrary', 'FirstLibrary.inf'), LibraryClassRecords)
        self.assertIn('Second.inf', ComponentRecords)
        self.assertNotIn(('POSTBUILD', 'python GenerateCapsule.py'), HeaderRecords)
        self.assertIn('POSTBUILD = python GenerateCapsule.py', ComponentRecords)

    def test_consecutive_nested_includes_inside_conditional_preserve_directive_state(self):
        self.WriteTmpFile(
            'First.dsc.inc',
            '[Components]\n'
            '!include Nested.dsc.inc\n'
            '[LibraryClasses]\n'
            '  FirstLibrary|FirstLibrary.inf\n',
        )
        self.WriteTmpFile(
            'Nested.dsc.inc',
            '[Components]\n'
            '  Nested.inf\n',
        )
        self.WriteTmpFile(
            'Second.dsc.inc',
            '[Components]\n'
            '  Second.inf\n'
            '!include Nested.dsc.inc\n',
        )
        self.WriteTmpFile(
            'Platform.dsc',
            '[Defines]\n'
            '  DEFINE INCLUDE_FEATURE = TRUE\n'
            '!if $(INCLUDE_FEATURE) == TRUE\n'
            '!include First.dsc.inc\n'
            '!include Second.dsc.inc\n'
            '  POSTBUILD = python GenerateCapsule.py\n'
            '!endif\n'
            '[LibraryClasses]\n'
            '  AfterConditional|AfterConditional.inf\n',
        )

        DscFile = PathClass(os.path.join(self.testDir, 'Platform.dsc'))
        Table = MetaFileStorage(
            WorkspaceDatabase(),
            DscFile,
            MODEL_FILE_DSC,
            True,
        )
        Parser = DscParser(DscFile, MODEL_FILE_DSC, TAB_ARCH_COMMON, Table)

        Headers = Parser[MODEL_META_DATA_HEADER, TAB_ARCH_COMMON]
        LibraryClasses = Parser[MODEL_EFI_LIBRARY_CLASS, TAB_ARCH_COMMON]
        Components = Parser[MODEL_META_DATA_COMPONENT, TAB_ARCH_COMMON]
        HeaderRecords = [(Record[0], Record[1]) for Record in Headers]
        LibraryClassRecords = [(Record[0], Record[1]) for Record in LibraryClasses]
        ComponentRecords = [Record[0] for Record in Components]

        self.assertIn(('FirstLibrary', 'FirstLibrary.inf'), LibraryClassRecords)
        self.assertIn(('AfterConditional', 'AfterConditional.inf'), LibraryClassRecords)
        self.assertNotIn(('POSTBUILD', 'python GenerateCapsule.py'), HeaderRecords)
        self.assertIn('Second.inf', ComponentRecords)
        self.assertIn('Nested.inf', ComponentRecords)
        self.assertIn('POSTBUILD = python GenerateCapsule.py', ComponentRecords)

    def test_reprocessing_after_include_resets_directive_stack_base(self):
        self.WriteTmpFile(
            'Included.dsc.inc',
            '[Components]\n'
            '  Included.inf\n',
        )
        self.WriteTmpFile(
            'Platform.dsc',
            '[Components]\n'
            '!include Included.dsc.inc\n',
        )

        DscFile = PathClass(os.path.join(self.testDir, 'Platform.dsc'))
        Table = MetaFileStorage(
            WorkspaceDatabase(),
            DscFile,
            MODEL_FILE_DSC,
            True,
        )
        Parser = DscParser(DscFile, MODEL_FILE_DSC, TAB_ARCH_COMMON, Table)

        Components = Parser[MODEL_META_DATA_COMPONENT, TAB_ARCH_COMMON]
        ComponentRecords = [Record[0] for Record in Components]
        self.assertIn('Included.inf', ComponentRecords)

        Parser.DoPostProcess()
        Components = Parser[MODEL_META_DATA_COMPONENT, TAB_ARCH_COMMON]
        self.assertEqual(ComponentRecords, [Record[0] for Record in Components])

    def test_parent_section_after_closed_component_subsection(self):
        self.WriteTmpFile(
            'Component.dsc.inc',
            '[Components]\n'
            '  Included.inf {\n'
            '    <LibraryClasses>\n'
            '      IncludedLibrary|IncludedLibrary.inf\n'
            '  }\n',
        )
        self.WriteTmpFile(
            'Platform.dsc',
            '[LibraryClasses]\n'
            '  BeforeInclude|Before.inf\n'
            '!include Component.dsc.inc\n'
            '[LibraryClasses]\n'
            '  AfterInclude|After.inf\n',
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
        LibraryClassRecords = [(Record[0], Record[1]) for Record in LibraryClasses]

        self.assertIn(('BeforeInclude', 'Before.inf'), LibraryClassRecords)
        self.assertIn(('AfterInclude', 'After.inf'), LibraryClassRecords)

    def test_nested_include_closes_component_subsection(self):
        self.WriteTmpFile(
            'NestedComponents.dsc.inc',
            '[Components]\n'
            '  Included.inf {\n'
            '    <LibraryClasses>\n'
            '      IncludedLibrary|IncludedLibrary.inf\n'
            '  }\n',
        )
        self.WriteTmpFile(
            'Included.dsc.inc',
            '[Components]\n'
            '!include NestedComponents.dsc.inc\n',
        )
        self.WriteTmpFile(
            'Platform.dsc',
            '[LibraryClasses]\n'
            '  BeforeInclude|Before.inf\n'
            '!include Included.dsc.inc\n'
            '[LibraryClasses]\n'
            '  AfterInclude|After.inf\n',
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
        LibraryClassRecords = [(Record[0], Record[1]) for Record in LibraryClasses]

        self.assertIn(('BeforeInclude', 'Before.inf'), LibraryClassRecords)
        self.assertIn(('AfterInclude', 'After.inf'), LibraryClassRecords)


def TheTestSuite():
    return unittest.TestLoader().loadTestsFromTestCase(TestDscParserInclude)


if __name__ == '__main__':
    unittest.TextTestRunner(verbosity=2).run(TheTestSuite())
