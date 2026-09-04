## @file
# Unit tests for DSC parser component scope handling.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import tempfile
import unittest

from Common import GlobalData
from CommonDataClass.DataClass import MODEL_EFI_LIBRARY_CLASS
from CommonDataClass.DataClass import MODEL_FILE_DSC
from CommonDataClass.DataClass import MODEL_META_DATA_COMPONENT
from CommonDataClass.DataClass import MODEL_PCD_FIXED_AT_BUILD
from Common.Misc import PathClass
from Workspace.MetaFileParser import DscParser
from Workspace.MetaFileParser import MetaFileParser
from Workspace.MetaFileTable import MetaFileStorage
from Workspace.WorkspaceDatabase import WorkspaceDatabase


class DscParserComponentScopeTests(unittest.TestCase):
    def setUp(self):
        self._saved_workspace = GlobalData.gWorkspace
        self._saved_global_defines = GlobalData.gGlobalDefines
        self._saved_platform_defines = GlobalData.gPlatformDefines
        self._saved_command_line_defines = GlobalData.gCommandLineDefines
        self._saved_edk_global = GlobalData.gEdkGlobal
        self._saved_platform_pcds = GlobalData.gPlatformPcds
        self._saved_platform_other_pcds = GlobalData.gPlatformOtherPcds
        self._saved_meta_files = MetaFileParser.MetaFiles.copy()
        self._saved_storage_cache = MetaFileStorage._ObjectCache.copy()
        self._saved_included_files = DscParser.IncludedFiles.copy()

        self._temporary_directory = tempfile.TemporaryDirectory()
        workspace = self._temporary_directory.name
        GlobalData.gWorkspace = workspace
        GlobalData.gGlobalDefines = {"WORKSPACE": workspace}
        GlobalData.gPlatformDefines = {}
        GlobalData.gCommandLineDefines = {}
        GlobalData.gEdkGlobal = {}
        GlobalData.gPlatformPcds = {}
        GlobalData.gPlatformOtherPcds = {}

    def _write_file(self, relative_path, content):
        path = os.path.join(self._temporary_directory.name, relative_path)
        directory = os.path.dirname(path)
        if directory:
            os.makedirs(directory, exist_ok=True)
        with open(path, "w", newline="\n") as output_file:
            output_file.write(content)
        return path

    def _create_parser(
        self,
        platform_name,
        platform_content,
        included_files,
        active_architecture="IA32",
    ):
        for relative_path, content in included_files.items():
            self._write_file(relative_path, content)

        platform_path = self._write_file(platform_name, platform_content)
        path = PathClass(platform_path)
        database = WorkspaceDatabase()
        table = MetaFileStorage(database, path, MODEL_FILE_DSC)
        return DscParser(path, MODEL_FILE_DSC, active_architecture, table)

    @staticmethod
    def _platform_content(
        include_path,
        define_architecture=True,
        macro_architecture="IA32",
    ):
        architecture_definition = (
            f"  DEFINE PEI_ARCH = {macro_architecture}\n"
            if define_architecture
            else ""
        )
        return (
            "[Defines]\n"
            "  PLATFORM_NAME = ParserTest\n"
            "  PLATFORM_GUID = 11111111-2222-3333-4444-555555555555\n"
            "  PLATFORM_VERSION = 0.1\n"
            "  DSC_SPECIFICATION = 0x00010005\n"
            "  OUTPUT_DIRECTORY = Build/ParserTest\n"
            "  SUPPORTED_ARCHITECTURES = IA32|X64\n"
            "  BUILD_TARGETS = DEBUG\n"
            "  SKUID_IDENTIFIER = DEFAULT\n"
            f"{architecture_definition}"
            "\n"
            f"!include {include_path}\n"
        )

    @staticmethod
    def _component_content(section):
        return (
            f"[{section}]\n"
            "  TestPkg/TestPeim.inf {\n"
            "    <LibraryClasses>\n"
            "      TestLib|TestPkg/TestLib.inf\n"
            "    <PcdsFixedAtBuild>\n"
            "      gTestPkgTokenSpaceGuid.PcdTest|TRUE\n"
            "  }\n"
        )

    def _assert_component_private_records(self, parser, architecture):
        components = parser[MODEL_META_DATA_COMPONENT, architecture]
        self.assertEqual(1, len(components))
        self.assertEqual(architecture, components[0][3])
        component_id = components[0][6]

        library_classes = parser[
            MODEL_EFI_LIBRARY_CLASS, architecture, None, component_id
        ]
        self.assertEqual(1, len(library_classes))
        self.assertEqual(architecture, library_classes[0][3])

        fixed_pcds = parser[
            MODEL_PCD_FIXED_AT_BUILD, architecture, None, component_id
        ]
        self.assertEqual(1, len(fixed_pcds))
        self.assertEqual(architecture, fixed_pcds[0][3])

        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, architecture],
        )
        self.assertEqual(
            [],
            parser[MODEL_PCD_FIXED_AT_BUILD, architecture],
        )

    def tearDown(self):
        MetaFileParser.MetaFiles.clear()
        MetaFileParser.MetaFiles.update(self._saved_meta_files)
        MetaFileStorage._ObjectCache.clear()
        MetaFileStorage._ObjectCache.update(self._saved_storage_cache)
        DscParser.IncludedFiles.clear()
        DscParser.IncludedFiles.update(self._saved_included_files)
        GlobalData.gWorkspace = self._saved_workspace
        GlobalData.gGlobalDefines = self._saved_global_defines
        GlobalData.gPlatformDefines = self._saved_platform_defines
        GlobalData.gCommandLineDefines = self._saved_command_line_defines
        GlobalData.gEdkGlobal = self._saved_edk_global
        GlobalData.gPlatformPcds = self._saved_platform_pcds
        GlobalData.gPlatformOtherPcds = self._saved_platform_other_pcds
        self._temporary_directory.cleanup()

    def test_private_records_use_expanded_component_architecture(self):
        parser = self._create_parser(
            "Platform.dsc",
            self._platform_content("Child.inc.dsc"),
            {
                "Child.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                )
            },
        )
        self._assert_component_private_records(parser, "IA32")

    def test_macro_architecture_is_independent_of_active_architecture(self):
        parser = self._create_parser(
            "DifferentArch.dsc",
            self._platform_content(
                "DifferentArch.inc.dsc",
                macro_architecture="X64",
            ),
            {
                "DifferentArch.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                )
            },
            active_architecture="IA32",
        )
        self._assert_component_private_records(parser, "X64")

        self.assertEqual(
            [],
            parser[MODEL_META_DATA_COMPONENT, "IA32"],
        )

    def test_aarch64_private_records_use_expanded_component_architecture(self):
        platform_content = self._platform_content(
            "Aarch64.inc.dsc",
            macro_architecture="AARCH64",
        ).replace(
            "SUPPORTED_ARCHITECTURES = IA32|X64",
            "SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64",
        )
        parser = self._create_parser(
            "Aarch64.dsc",
            platform_content,
            {
                "Aarch64.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                )
            },
            active_architecture="AARCH64",
        )
        self._assert_component_private_records(parser, "AARCH64")

    def test_literal_component_architecture_is_unchanged(self):
        parser = self._create_parser(
            "Literal.dsc",
            self._platform_content("Literal.inc.dsc", False),
            {
                "Literal.inc.dsc": self._component_content(
                    "Components.IA32"
                )
            },
        )
        self._assert_component_private_records(parser, "IA32")

    def test_unresolved_component_architecture_macro_is_unchanged(self):
        parser = self._create_parser(
            "Unresolved.dsc",
            self._platform_content("Unresolved.inc.dsc", False),
            {
                "Unresolved.inc.dsc": self._component_content(
                    "Components.$(UNKNOWN_ARCH)"
                )
            },
        )
        self._assert_component_private_records(parser, "$(UNKNOWN_ARCH)")

    def test_nested_include_private_records_use_expanded_architecture(self):
        parser = self._create_parser(
            "Nested.dsc",
            self._platform_content("Intermediate.inc.dsc"),
            {
                "Intermediate.inc.dsc": "!include Leaf.inc.dsc\n",
                "Leaf.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                ),
            },
        )
        self._assert_component_private_records(parser, "IA32")

    def test_top_level_include_scope_is_not_expanded(self):
        parser = self._create_parser(
            "TopLevelInclude.dsc",
            self._platform_content("TopLevelInclude.inc.dsc"),
            {
                "TopLevelInclude.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH)]\n"
                    "  TestLib|TestPkg/TestLib.inf\n"
                )
            },
        )

        library_classes = parser[
            MODEL_EFI_LIBRARY_CLASS,
            "$(PEI_ARCH)",
        ]
        self.assertEqual(1, len(library_classes))
        self.assertEqual("$(PEI_ARCH)", library_classes[0][3])
        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, "IA32"],
        )

    def test_reprocessing_does_not_reuse_component_owner_mapping(self):
        GlobalData.gCommandLineDefines["ENABLE_INCLUDE"] = "FALSE"
        platform_content = self._platform_content("Reprocess.inc.dsc").replace(
            "!include Reprocess.inc.dsc\n",
            (
                "!if $(ENABLE_INCLUDE) == TRUE\n"
                "!include Reprocess.inc.dsc\n"
                "!endif\n"
            ),
        )
        parser = self._create_parser(
            "Reprocess.dsc",
            platform_content,
            {
                "Reprocess.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH)]\n"
                    "  TestLib|TestPkg/TestLib.inf\n"
                )
            },
        )

        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, "$(PEI_ARCH)"],
        )

        GlobalData.gCommandLineDefines["ENABLE_INCLUDE"] = "TRUE"
        parser.DoPostProcess()

        library_classes = parser[
            MODEL_EFI_LIBRARY_CLASS,
            "$(PEI_ARCH)",
        ]
        self.assertEqual(1, len(library_classes))
        self.assertEqual("$(PEI_ARCH)", library_classes[0][3])
        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, "IA32"],
        )

    def test_private_subsection_include_keeps_expanded_architecture(self):
        parser = self._create_parser(
            "SubsectionInclude.dsc",
            self._platform_content("SubsectionInclude.inc.dsc"),
            {
                "SubsectionInclude.inc.dsc": (
                    "[Components.$(PEI_ARCH), Components.X64]\n"
                    "  TestPkg/TestPeim.inf {\n"
                    "    <LibraryClasses>\n"
                    "      !include PrivateLibrary.inc.dsc\n"
                    "    <PcdsFixedAtBuild>\n"
                    "      gTestPkgTokenSpaceGuid.PcdTest|TRUE\n"
                    "  }\n"
                ),
                "PrivateLibrary.inc.dsc": (
                    "TestLib|TestPkg/TestLib.inf\n"
                ),
            },
        )
        self._assert_component_private_records(parser, "IA32")
        self._assert_component_private_records(parser, "X64")

    def test_multi_architecture_private_records_keep_matching_owners(self):
        parser = self._create_parser(
            "MultiArch.dsc",
            self._platform_content("MultiArch.inc.dsc"),
            {
                "MultiArch.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH), Components.X64"
                )
            },
        )
        self._assert_component_private_records(parser, "IA32")
        self._assert_component_private_records(parser, "X64")

    def test_repeated_macro_sections_keep_private_records_with_their_owner(self):
        parser = self._create_parser(
            "RepeatedSections.dsc",
            self._platform_content("RepeatedSections.inc.dsc"),
            {
                "RepeatedSections.inc.dsc": (
                    "[Components.$(PEI_ARCH)]\n"
                    "  TestPkg/FirstPeim.inf\n"
                    "\n"
                    + self._component_content("Components.$(PEI_ARCH)")
                )
            },
        )

        components = parser[MODEL_META_DATA_COMPONENT, "IA32"]
        self.assertEqual(
            ["TestPkg/FirstPeim.inf", "TestPkg/TestPeim.inf"],
            [component[0] for component in components],
        )
        first_component_id = components[0][6]
        private_component_id = components[1][6]

        self.assertEqual(
            [],
            parser[
                MODEL_EFI_LIBRARY_CLASS,
                "IA32",
                None,
                first_component_id,
            ],
        )
        self.assertEqual(
            1,
            len(
                parser[
                    MODEL_EFI_LIBRARY_CLASS,
                    "IA32",
                    None,
                    private_component_id,
                ]
            ),
        )
        self.assertEqual(
            1,
            len(
                parser[
                    MODEL_PCD_FIXED_AT_BUILD,
                    "IA32",
                    None,
                    private_component_id,
                ]
            ),
        )


def TheTestSuite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
        DscParserComponentScopeTests
    )


if __name__ == "__main__":
    unittest.main()
